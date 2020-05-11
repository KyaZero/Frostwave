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
		void AddLight(DirectionalLight* light);
		void SetCamera(Camera* camera);
		void Submit(RenderManager* renderer);

		Camera* GetCamera() const { return m_Camera; }

	private:
		Camera* m_Camera;
		std::vector<Model*> m_Models;
		std::vector<PointLight*> m_PointLights;
		std::vector<DirectionalLight*> m_DirectionalLights;
	};
}
namespace fw = frostwave;