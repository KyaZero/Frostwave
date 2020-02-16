#pragma once
#include <Engine/Core/Math/Mat4.h>
#include <Engine/Core/Math/Vec.h>
#include <Engine/Graphics/Model.h>
#include <Engine/Platform/Window.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Graphics/Sampler.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Graphics/Lights.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Graphics/RenderStateManager.h>

namespace frostwave
{
	class DeferredRenderer
	{
	public:
		DeferredRenderer();
		~DeferredRenderer();

		void Init();
		void RenderGeometry(f32 totalTime, Camera* camera);
		void RenderLighting(f32 totalTime, RenderStateManager* stateManager);
		void Submit(Model* model);
		void Submit(PointLight* light);
		void Submit(DirectionalLight* light);
		void Submit(EnvironmentLight* light);

	private:
		std::vector<Model*> m_Models;
		std::vector<PointLight*> m_PointLights;

		Buffer m_GeometryFrameBuffer, m_ObjectBuffer, m_LightingBuffer;
		Shader m_RenderGeometryShader, m_PointLightShader, m_EnvLightShader;
		EnvironmentLight* m_EnvironmentLight;
		DirectionalLight* m_DirectionalLight;
		Model* m_LightSphere;
		Texture m_NullTexture;

		struct GeometryFrameBuffer
		{
			Mat4f view;
			Mat4f projection;
			Mat4f invProjection;
			Mat4f invView;
			Vec4f cameraPos;
			float nearZ;
			float farZ;
			Vec2f resolution;
		} m_GeometryFrameBufferData;

		struct ObjectBuffer
		{
			Mat4f model;
			Material material;
		} m_ObjectBufferData;

		struct LightingBuffer
		{
			Mat4f lightMatrix;
			struct 
			{
				Vec4f position; //radius in alpha channel
				Vec4f color; //intensity in alpha channel
			} pointLight;
			struct
			{
				Vec4f direction;
				Vec4f color; //intensity in alpha channel
			} dirLight;
		} m_LightingBufferData;
	};
}
namespace fw = frostwave;