#include "GBuffer.h"
#include <Engine/Graphics/Framework.h>
#include <Engine/Memory/Allocator.h>
#include <Engine/Graphics/Error.h>
#include <d3d11.h>

frostwave::GBuffer::GBuffer()
{
	for (i32 i = 0; i < m_Textures.size(); ++i)
	{
		m_Textures[i] = Allocate();
	}
}

frostwave::GBuffer::~GBuffer()
{
	for (i32 i = 0; i < m_Textures.size(); ++i)
	{
		Free(m_Textures[i]);
	}
}

void frostwave::GBuffer::Create(Vec2i size)
{
	std::array<ImageFormat, (i32)GBuffer::Textures::Count> textureFormats =
	{
		(ImageFormat)DXGI_FORMAT_R8G8B8A8_UNORM, //Albedo
		(ImageFormat)DXGI_FORMAT_R16G16B16A16_SNORM, //Normals
		(ImageFormat)DXGI_FORMAT_R8G8B8A8_UNORM, //Roughness, Metallic, Ambient Occlusion
		(ImageFormat)DXGI_FORMAT_R32_FLOAT //Emissive
	};

	i32 i = 0;
	for (auto* tex : m_Textures)
	{
		tex->Create(size, textureFormats[i++]);
	}
}

void frostwave::GBuffer::ClearTexture(Vec4f clear)
{
	for (auto* tex : m_Textures)
	{
		tex->Clear(clear);
	}
}

void frostwave::GBuffer::SetAsActiveTarget(Texture* depth)
{
	std::array<ID3D11RenderTargetView*, (i32)Textures::Count> rts;
	i32 i = 0;
	for (auto* tex : m_Textures)
	{
		rts[i++] = tex->GetRenderTarget();
		tex->SetViewport();
	}
	Framework::GetContext()->OMSetRenderTargets((u32)rts.size(), rts.data(), depth->GetDepth());
}

frostwave::Texture* frostwave::GBuffer::GetTexture(Textures resource)
{
	return m_Textures[(i32)resource];
}

void frostwave::GBuffer::SetAsResourceOnSlot(Textures resource, u32 slot)
{
	m_Textures[(i32)resource]->Bind(slot);
}

void frostwave::GBuffer::SetAllAsResources()
{
	std::array<ID3D11ShaderResourceView*, (i32)Textures::Count> srvs;
	i32 i = 0;
	for (auto* tex : m_Textures)
	{
		srvs[i++] = tex->GetShaderResourceView();
	}
	Framework::GetContext()->PSSetShaderResources(1, (i32)Textures::Count, srvs.data());
}

void frostwave::GBuffer::RemoveAllAsResources()
{
	ID3D11ShaderResourceView* nullSRV[(i32)Textures::Count] = { nullptr };
	Framework::GetContext()->PSSetShaderResources(1, (i32)Textures::Count, nullSRV);
}

void frostwave::GBuffer::Release()
{
	for (auto* tex : m_Textures)
	{
		if (tex)
			SafeRelease(&tex);
	}
}