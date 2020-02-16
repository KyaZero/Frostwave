#include "Scene.h"
#include <Engine/Memory/Allocator.h>

frostwave::Scene::Scene() : m_DirectionalLight(nullptr), m_EnvironmentLight(nullptr)
{
}

frostwave::Scene::~Scene()
{
	if (m_DirectionalLight)
		Free(m_DirectionalLight);

	if (m_EnvironmentLight)
		Free(m_EnvironmentLight);

	for (auto* m : m_Models)
	{
		Free(m);
	}

	for (auto* l : m_PointLights)
	{
		Free(l);
	}

	m_Models.clear();
}

void frostwave::Scene::Init()
{
	m_DirectionalLight = nullptr;
	m_EnvironmentLight = nullptr;
	m_Camera = nullptr;
}

void frostwave::Scene::AddModel(Model* model)
{
	m_Models.push_back(model);
}

void frostwave::Scene::AddLight(PointLight* light)
{
	m_PointLights.push_back(light);
}

void frostwave::Scene::SetCamera(Camera* camera)
{
	m_Camera = camera;
}

void frostwave::Scene::Submit(RenderManager* renderer)
{
	for (auto* model : m_Models)
		renderer->Submit(model);
	for (auto* light : m_PointLights)
		renderer->Submit(light);
	renderer->Submit(m_EnvironmentLight);
	renderer->Submit(m_DirectionalLight);
}
