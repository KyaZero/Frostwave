#pragma once
#include <Engine/Core/Types.h>
#include <Engine/Core/Math/Vec.h>
#include <Engine/Graphics/ImageFormat.h>
#include <string>

struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

namespace frostwave
{
	struct TextureCreateInfo
	{
		Vec2i size;
		ImageFormat format = ImageFormat::DXGI_FORMAT_R8G8B8A8_UNORM;
		void* data = nullptr;
		bool renderTarget = true;
		bool hdr = false; 
		bool cubemap = false; 
		u32 numMips = 1;
	};

	class Texture
	{
	public:
		Texture();
		Texture(const std::string& path);
		//Helper function for simple textures
		Texture(Vec2i size, ImageFormat format = ImageFormat::DXGI_FORMAT_R8G8B8A8_UNORM, void* data = nullptr);
		Texture(const TextureCreateInfo& info);
		Texture(const Texture& other);
		Texture(Texture&& other);

		Texture& operator=(const Texture& other);
		Texture& operator=(Texture&& other);
		~Texture();

		bool Valid();

		bool Load(const std::string& path);
		//Helper function for simple textures
		void Create(Vec2i size, ImageFormat format = ImageFormat::DXGI_FORMAT_R8G8B8A8_UNORM, void* data = nullptr);
		void Create(const TextureCreateInfo& info);
		void CreateFromTexture(ID3D11Texture2D* texture);
		void CreateDepth(Vec2i size, ImageFormat format = ImageFormat::DXGI_FORMAT_D32_FLOAT);

		void Clear(Vec4f color = Vec4f());
		void ClearDepth(f32 depth = 1.0f, u32 stencil = 0);
		void SetAsActiveTarget(Texture* depth = nullptr);
		void SetViewport();
		void SetCustomViewport(f32 topLeftX, f32 topLeftY, f32 width, f32 height, f32 minDepth = 0.0f, f32 maxDepth = 1.0f);
		static void UnsetActiveTarget();
		static void Unbind(u32 slot);
		static void UnbindAll();
		void Bind(u32 slot) const;
		void Release();

		//Note: Don't forget to call Release on the render target view after using it, as it would leak memory otherwise.
		ID3D11RenderTargetView* CreateRenderTargetViewForMip(i32 mipLevel, bool cubemap = false);

		ID3D11Texture2D* GetTexture() const;
		ID3D11DepthStencilView* GetDepth() const;
		ID3D11RenderTargetView* GetRenderTarget() const;
		ID3D11ShaderResourceView* GetShaderResourceView() const;
		Vec2i GetSize() const;

	private:
		struct Data;
		Data* m_Data;
	};
}
namespace fw = frostwave;