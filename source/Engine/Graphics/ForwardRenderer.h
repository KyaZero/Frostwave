#pragma once
#include <Engine/Core/Math/Mat4.h>
#include <Engine/Core/Math/Vec.h>
#include <Engine/Graphics/Model.h>
#include <Engine/Platform/Window.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Graphics/Lights.h>
#include <vector>

namespace frostwave
{
	class ForwardRenderer
	{
	public:
		ForwardRenderer();
		virtual ~ForwardRenderer();

		void Init();
		void Render(f32 totalTime, Camera* camera);
		void Submit(Model* model);
		void Submit(const PointLight& light);
		void Submit(Texture* envMap);

	private:
		std::vector<Model*> m_Models;
		std::vector<PointLight> m_Lights;
		Buffer m_FrameBuffer, m_ObjectBuffer;

		Texture m_NullTexture;

		struct FrameBuffer
		{
			Mat4f view;
			Mat4f projection;
			Vec4f cameraPos;
			Vec4f lightDir;
			Vec4f lightColor;

			PointLight lights[32];
		} m_FrameBufferData;

		struct ObjectBuffer
		{
			Mat4f model;
			Material material;
		} m_ObjectBufferData;

		Texture* m_EnvironmentMap;
	};
}