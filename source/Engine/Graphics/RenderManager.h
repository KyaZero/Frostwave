#pragma once
#include <Engine/Core/Types.h>
#include <Engine/Graphics/Lights.h>
#include <Engine/Graphics/GBuffer.h>
#include <Engine/Graphics/RenderStateManager.h>
#include <Engine/Graphics/Sampler.h>

namespace frostwave
{
	class Camera;
	class Texture;
	class ForwardRenderer;
	class DeferredRenderer;
	class ShadowRenderer;
	class PostProcessor;
	class Model;
	class RenderManager
	{
	public:
		RenderManager();
		~RenderManager();

		void Init();
		void Render(f32 totalTime, Camera* camera, Texture* backBuffer);

		void ResizeTextures(i32 width, i32 height);

		void Submit(Model* model);
		void Submit(PointLight* light);
		void Submit(DirectionalLight* light);
		void Submit(EnvironmentLight* light);

	private:
		void ClearTextures();
		void InitPostProcessing();

		Texture* m_IntermediateTexture;
		Texture* m_IntermediateTexture2;
		Texture* m_IntermediateTexture3;
		Texture* m_ResolveHDRTexture;
		Texture* m_HalfSizeTexturePing;
		Texture* m_HalfSizeTexturePong;
		Texture* m_QuarterSizeTexturePing;
		Texture* m_QuarterSizeTexturePong;
		Texture* m_BloomTexture;
		Texture* m_IntermediateDepth;
		Texture* m_SSAONoiseTexture;
		GBuffer* m_GBuffer;
		ForwardRenderer* m_ForwardRenderer;
		DeferredRenderer* m_DeferredRenderer;
		ShadowRenderer* m_ShadowRenderer;
		PostProcessor* m_PostProcessor;
		RenderStateManager m_StateManager;
		DirectionalLight* m_DirectionalLight;
		Sampler* m_LinearWrapSampler,* m_PointWrapSampler,* m_PointClampSampler,* m_LinearClampSampler;
	};
}
namespace fw = frostwave;