#pragma once
#include <Engine/Core/Types.h>
#include <Engine/Core/Math/Vec.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Memory/Allocator.h>
#include <Engine/Core/Math/Mat4.h>

namespace frostwave
{
	class BaseLight
	{
	public:
		BaseLight() : m_Color(Vec3f(1,1,1)), m_Intensity(1.0f) { }
		BaseLight(Vec3f color, f32 intensity = 1.0f) : m_Color(color), m_Intensity(intensity) { }

		virtual ~BaseLight() { }

		void SetColor(const Vec3f color) { m_Color = color; }
		const Vec3f& GetColor() const { return m_Color; }

		void SetIntensity(f32 intensity) { m_Intensity = intensity; }
		f32 GetIntensity() const { return m_Intensity; }

	private:
		Vec3f m_Color;
		f32 m_Intensity;
	};

	class PointLight : public BaseLight
	{
	public:
		PointLight() : BaseLight(), m_Position(Vec3f()), m_Radius(0) { }
		PointLight(Vec3f position, f32 radius) : BaseLight(), m_Position(position), m_Radius(radius) { }
		PointLight(Vec3f position, f32 radius, Vec3f color, f32 intensity = 1.0f) : BaseLight(color, intensity), m_Position(position), m_Radius(radius) { }

		virtual ~PointLight() { }

		const Vec3f& GetPosition() const { return m_Position; }
		f32 GetRadius() const { return m_Radius; }

	private:
		Vec3f m_Position;
		f32 m_Radius;
	};

	struct DirectionalLightShadowData
	{
		Mat4f ViewProj;
		Texture* shadowMap = nullptr;
		Texture* depth = nullptr;
	};

	class DirectionalLight : public BaseLight
	{
	public:
		DirectionalLight() : BaseLight(), m_Direction(Vec3f(1, 1, 1)) { }
		DirectionalLight(Vec3f direction) : BaseLight(), m_Direction(direction) { }
		DirectionalLight(Vec3f direction, Vec3f color, f32 intensity = 1.0f) : BaseLight(color, intensity), m_Direction(direction) { }

		virtual ~DirectionalLight() { if(m_ShadowData.depth) Free(m_ShadowData.depth); if (m_ShadowData.shadowMap) Free(m_ShadowData.shadowMap); }

		void SetDirection(const Vec3f& direction) { m_Direction = direction; }
		const Vec3f& GetDirection() const { return m_Direction; }
		DirectionalLightShadowData& GetShadowData() { return m_ShadowData; }

	private:
		Vec3f m_Direction;

		friend class ShadowRenderer;
		DirectionalLightShadowData m_ShadowData;
	};

	class EnvironmentLight : public BaseLight
	{
	public:
		EnvironmentLight() : BaseLight(), m_Cubemap(nullptr) { }
		EnvironmentLight(Texture* cubemap) : BaseLight(), m_Cubemap(cubemap) { }
		EnvironmentLight(Texture* cubemap, Vec3f color, f32 intensity = 1.0f) : BaseLight(color, intensity), m_Cubemap(cubemap) { }

		virtual ~EnvironmentLight() { if(m_Cubemap) Free(m_Cubemap); }

		Texture* GetCubemap() const { return m_Cubemap; }

	private:
		Texture* m_Cubemap;
	};
}