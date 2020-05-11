#pragma once
#include <Engine/Graphics/Shader.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/Buffer.h>
#include <Engine/Core/Math/Mat4.h>
#include <Engine/Graphics/Lights.h>
#include <Engine/Graphics/Camera.h>
#include <string>
#include <vector>

namespace frostwave
{
	struct PostProcessStage
	{
		PostProcessStage() { }
		PostProcessStage(const std::string& pixelShader, const std::vector<Texture*>& shaderResourceView, Texture* renderTarget, const std::string& name, bool renderToBackbuffer = false) :
			pixelShader(Allocate<Shader>(Shader::Type::Pixel, pixelShader)), shaderResources(shaderResourceView), renderTarget(renderTarget), name(name), renderToBackbuffer(renderToBackbuffer) { }

		Shader* pixelShader;
		std::vector<Texture*> shaderResources;
		Texture* renderTarget;
		std::string name;
		bool renderToBackbuffer;
	};

	class Technique
	{
	public:
		void Push(PostProcessStage stage) { stages.push_back(stage); }

		std::string name;
	private:
		friend class PostProcessor;
		std::vector<PostProcessStage> stages;
	};

	class PostProcessor
	{
	public:
		PostProcessor();
		~PostProcessor();

		void Init();
		void Render(Texture* backBuffer, Camera* camera, DirectionalLight* light);
		void Push(PostProcessStage stage);
		void Push(Technique tech);

		void Clear();

	private:
		void RenderStage(PostProcessStage stage, Texture* backBuffer);

		struct FrameBuffer
		{
			Mat4f view;
			Mat4f projection;
			Mat4f invProjection;
			Mat4f invView;
			Mat4f lightMatrix;
			Vec4f cameraPos;
			Vec4f lightDirection;
			Vec4f lightColor;
			Vec4f kernel[16];
			Vec2f resolution;
			Vec2f texelSize;
			Vec2f size;
			f32 farPlane;
			f32 nearPlane;
		} m_FrameBufferData;

		Buffer m_FrameBuffer;
		Shader* m_FullscreenVertexShader;
		std::vector<Technique> m_Techniques;
		std::vector<Vec4f> m_SSAOKernel;
	};
}
namespace fw = frostwave;