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

		Texture* GenerateCubemap(Texture* hdriTexture);
		void PrefilterPBRTextures(Texture* environmentMap);

		void RenderGeometry(f32 totalTime, Camera* camera);
		void RenderLighting(f32 totalTime, RenderStateManager* stateManager);

		void Submit(Model* model);
		void Submit(PointLight* light);
		void Submit(DirectionalLight* light);

	private:
		void ConvoluteCubemap(Texture* environmentMap);
		void PrefilterSpecularCubemap(Texture* environmentMap);
		void GenerateBRDFTexture();

		std::vector<Model*> m_Models;
		std::vector<PointLight*> m_PointLights;
		std::vector<DirectionalLight*> m_DirectionalLights;

		Buffer m_GeometryFrameBuffer, m_ObjectBuffer, m_LightingBuffer;
		Shader m_RenderGeometryShader, m_PointLightShader, m_AmbientLightShader, m_DirectionalLightShader;
		Model* m_LightSphere;
		Texture m_NullTexture;
		Texture* m_IrradianceTexture;
		Texture* m_PrefilteredTexture;
		Texture* m_BRDFTexture;

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