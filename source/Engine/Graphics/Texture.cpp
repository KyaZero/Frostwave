#include "Texture.h"
#include <Engine/Memory/Allocator.h>
#include <Engine/Graphics/Textures/DDSTextureLoader.h>
#include <Engine/Graphics/Framework.h>
#include <Engine/Graphics/Error.h>
#include <Engine/Core/Math/Vec2.h>
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <d3d11.h>

struct frostwave::Texture::Data
{
	std::string path = "";
	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* shaderResource = nullptr;
	ID3D11DepthStencilView* depth = nullptr;
	ID3D11RenderTargetView* renderTarget = nullptr;
	Vec2i size = Vec2i();
	D3D11_VIEWPORT viewport = {};
	bool isDepth = false;
};

frostwave::Texture::Texture() : m_Data(nullptr)
{
	m_Data = Allocate();
}

frostwave::Texture::Texture(const std::string& path) : m_Data(nullptr)
{
	Load(path);
}

frostwave::Texture::Texture(Vec2i size, ImageFormat format, void* data, bool renderTarget) : m_Data(nullptr)
{
	Create(size, format, data, renderTarget);
}

frostwave::Texture::Texture(const Texture& other) : m_Data(nullptr)
{
	operator=(other);
}

frostwave::Texture::Texture(Texture&& other) : m_Data(nullptr)
{
	operator=(std::forward<Texture>(other));
}

frostwave::Texture& frostwave::Texture::operator=(const Texture& other)
{
	if (m_Data)
	{
		Release();
		Free(m_Data);
	}
	Load(other.m_Data->path);
	return *this;
}

frostwave::Texture& frostwave::Texture::operator=(Texture&& other)
{
	if (m_Data)
	{
		Release();
		Free(m_Data);
	}
	m_Data = other.m_Data;
	other.m_Data = nullptr;
	return *this;
}

frostwave::Texture::~Texture()
{
	Release();
	if (m_Data)
	{
		Free(m_Data);
	}
}

bool frostwave::Texture::Valid()
{
	return m_Data && (m_Data->shaderResource || m_Data->texture || m_Data->depth|| m_Data->renderTarget);
}

void frostwave::Texture::SetAsActiveTarget(frostwave::Texture* depthStencil)
{
	Framework::GetContext()->OMSetRenderTargets(1, &m_Data->renderTarget, depthStencil ? depthStencil->m_Data->depth : nullptr);
	SetViewport();
}

void frostwave::Texture::SetViewport()
{
	Framework::GetContext()->RSSetViewports(1, &m_Data->viewport);
}

void frostwave::Texture::UnsetActiveTarget()
{
	Framework::GetContext()->OMSetRenderTargets(0, nullptr, nullptr);
}

void frostwave::Texture::Unbind(u32 slot)
{
	ID3D11ShaderResourceView* view = nullptr;
	Framework::GetContext()->PSSetShaderResources(slot, 1, &view);
}

void frostwave::Texture::UnbindAll()
{
	for (u32 i = 0; i < 16; i++)
	{
		Unbind(i);
	}
}

bool frostwave::Texture::Load(const std::string& path)
{
	if (path.length() <= 0) return false;
	if (!std::filesystem::exists(std::filesystem::path(path)))
	{
		ERROR_LOG("Texture file not found: %s", path.c_str());
		return false;
	}
	if (!m_Data)
		m_Data = Allocate();

	m_Data->path = path;

	if (m_Data->path.find(".dds") != std::string::npos || m_Data->path.find(".DDS") != std::string::npos)
	{
		ErrorCheck(DirectX::CreateDDSTextureFromFile(Framework::GetDevice(), std::wstring(m_Data->path.begin(), m_Data->path.end()).c_str(), nullptr, &m_Data->shaderResource));
	}
	else
	{
		i32 w, h, channels;
		unsigned char* image = stbi_load(m_Data->path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
		if (image != nullptr)
			Create({ w, h }, ImageFormat::DXGI_FORMAT_R8G8B8A8_UNORM, image);
		else
		{
			ERROR_LOG("Failed to load %s", m_Data->path.c_str());
			return false;
		}
		stbi_image_free(image);
	}
	return true;
}

void frostwave::Texture::Create(Vec2i size, ImageFormat format, void* data, bool renderTarget)
{
	if (!m_Data)
		m_Data = Allocate();

	D3D11_TEXTURE2D_DESC desc = { };
	desc.Width = (u32)size.x;
	desc.Height = (u32)size.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = (DXGI_FORMAT)format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;

	if (renderTarget)
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	else
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	ID3D11Texture2D* texture = nullptr;

	D3D11_SUBRESOURCE_DATA initialData = {};
	initialData.pSysMem = data;
	initialData.SysMemPitch = size.x * 4;

	ErrorCheck(Framework::GetDevice()->CreateTexture2D(&desc, data ? &initialData : nullptr, &texture));

	CreateFromTexture(texture, renderTarget);
	m_Data->size = size;
}

void frostwave::Texture::CreateFromTexture(ID3D11Texture2D* texture, bool renderTarget)
{
	if (renderTarget)
		ErrorCheck(Framework::GetDevice()->CreateRenderTargetView(texture, nullptr, &m_Data->renderTarget));
	if (texture)
	{
		D3D11_TEXTURE2D_DESC desc;
		texture->GetDesc(&desc);
		m_Data->viewport = { 0.0f, 0.0f, (f32)desc.Width, (f32)desc.Height, 0.0f, 1.0f };
		m_Data->size = { (i32)desc.Width, (i32)desc.Height };
	}
	m_Data->texture = texture;
	ErrorCheck(Framework::GetDevice()->CreateShaderResourceView(texture, nullptr, &m_Data->shaderResource));
}

void frostwave::Texture::CreateDepth(Vec2i size, ImageFormat format)
{
	D3D11_TEXTURE2D_DESC desc = { };
	desc.Width = (u32)size.x;
	desc.Height = (u32)size.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32_TYPELESS; (DXGI_FORMAT)format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
	dsv_desc.Flags = 0;
	dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
	dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Texture2D.MipSlice = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC sr_desc;
	sr_desc.Format = DXGI_FORMAT_R32_FLOAT;
	sr_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sr_desc.Texture2D.MostDetailedMip = 0;
	sr_desc.Texture2D.MipLevels = 1;

	ID3D11Texture2D* texture = nullptr;
	ErrorCheck(Framework::GetDevice()->CreateTexture2D(&desc, nullptr, &texture));
	ErrorCheck(Framework::GetDevice()->CreateDepthStencilView(texture, &dsv_desc, &m_Data->depth));
	ErrorCheck(Framework::GetDevice()->CreateShaderResourceView(texture, &sr_desc, &m_Data->shaderResource));

	m_Data->isDepth = true;
	m_Data->texture = texture;
	m_Data->viewport = { 0.0f, 0.0f, (f32)size.x, (f32)size.y, 0.0f, 1.0f };
	m_Data->size = size;
}

void frostwave::Texture::Clear(Vec4f color)
{
	Framework::GetContext()->ClearRenderTargetView(m_Data->renderTarget, &color.x);
}

void frostwave::Texture::ClearDepth(f32 depth, u32 stencil)
{
	Framework::GetContext()->ClearDepthStencilView(m_Data->depth, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, (UINT8)stencil);
}

void frostwave::Texture::Bind(u32 slot) const
{
	Framework::GetContext()->PSSetShaderResources(slot, 1, &m_Data->shaderResource);
}

void frostwave::Texture::Release()
{
	if (m_Data)
	{
		if (m_Data->depth)
			SafeRelease(&m_Data->depth);

		if (m_Data->renderTarget)
			SafeRelease(&m_Data->renderTarget);

		if (m_Data->shaderResource)
			SafeRelease(&m_Data->shaderResource);

		if (m_Data->texture)
			SafeRelease(&m_Data->texture);
	}
}

ID3D11Texture2D* frostwave::Texture::GetTexture() const
{
	return m_Data->texture;
}

ID3D11DepthStencilView* frostwave::Texture::GetDepth() const
{
	return m_Data->depth;
}

ID3D11RenderTargetView* frostwave::Texture::GetRenderTarget() const
{
	return m_Data->renderTarget;
}

ID3D11ShaderResourceView* frostwave::Texture::GetShaderResourceView() const
{
	return m_Data->shaderResource;
}

frostwave::Vec2i frostwave::Texture::GetSize() const
{
	return m_Data->size;
}
