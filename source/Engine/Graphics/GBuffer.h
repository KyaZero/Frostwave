#pragma once
#include <Engine/Core/Types.h>
#include <Engine/Core/Math/Vec4.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/Shader.h>
#include <array>

namespace frostwave
{
	class GBuffer
	{
	public:
		enum class Textures
		{
			Albedo,
			Normal,
			RMAO,
			Emissive,
			Count
		};

		GBuffer();
		~GBuffer();

		void Create(Vec2i size);

		void ClearTexture(Vec4f clear = Vec4f());

		void SetAsActiveTarget(Texture* depth = nullptr);

		Texture* GetTexture(Textures resource);

		void SetAsResourceOnSlot(Textures resource, u32 aSlot);
		void SetAllAsResources();
		void RemoveAllAsResources();
		void Release();

	private:
		std::array<Texture*, (i32)Textures::Count> m_Textures;
	};
}