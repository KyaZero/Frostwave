#pragma once
#include <Engine/Core/Math/Mat.h>
#include <Engine/Graphics/Model.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Graphics/Lights.h>
#include <vector>

namespace frostwave
{
	class ShadowRenderer
	{
	public:
		ShadowRenderer();
		virtual ~ShadowRenderer();

		void Init();
		void Render(Camera* camera);
		void Submit(Model* model);
		void Submit(DirectionalLight* light);

	private:
		std::vector<Model*> m_Models;
		std::vector<DirectionalLight*> m_DirectionalLights;
		Buffer m_FrameBuffer, m_ObjectBuffer;
		Shader m_ShadowShader;

		struct FrameBuffer
		{
			Mat4f VP;
		} m_FrameBufferData;

		struct ObjectBuffer
		{
			Mat4f model;
		} m_ObjectBufferData;
	};
}