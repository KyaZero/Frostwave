#pragma once
#include <Engine/Graphics/Model.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Graphics/RenderManager.h>
#include <Engine/Graphics/Lights.h>

namespace frostwave
{
	class Scene
	{
	public:
		Scene();
		~Scene();

		void Init();
		void AddModel(Model* model);
		void AddLight(PointLight* light);
		void SetCamera(Camera* camera);
		void Submit(RenderManager* renderer);

		void SetEnvironmentLight(EnvironmentLight* light)
		{
			m_EnvironmentLight = light;
		}

		void SetDirectionalLight(DirectionalLight* light)
		{
			m_DirectionalLight = light;
		}

		DirectionalLight* GetDirectionalLight() const { return m_DirectionalLight; }

		Camera* GetCamera() const { return m_Camera; }

	private:
		Camera* m_Camera;
		std::vector<Model*> m_Models;
		std::vector<PointLight*> m_PointLights;
		DirectionalLight* m_DirectionalLight;
		EnvironmentLight* m_EnvironmentLight;
	};
}
namespace fw = frostwave;