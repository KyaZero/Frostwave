#pragma once
#include <Engine/Core/Types.h>
#include <Engine/Graphics/Shader.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Graphics/Buffer.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/Model.h>

namespace frostwave
{
	class SkyboxRenderer
	{
	public:
		SkyboxRenderer();
		virtual ~SkyboxRenderer();

		void Init();
		void SetTexture(Texture* skyboxTexture);
		void Render(f32 totalTime, Camera* camera);

	private:
		Texture* m_SkyboxTexture;

		Shader* m_SkyboxShader;
		Buffer* m_FrameBuffer;
		Model* m_Cube;

		struct FrameBuffer
		{
			Mat4f view;
			Mat4f projection;
		} m_FrameBufferData;
	};
}