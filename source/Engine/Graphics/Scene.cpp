#include "Scene.h"
#include <Engine/Memory/Allocator.h>

frostwave::Scene::Scene()
{
}

frostwave::Scene::~Scene()
{
	for (auto* model : m_Models)
		Free(model);

	for (auto* light : m_PointLights)
		Free(light);

	for (auto* light : m_DirectionalLights)
		Free(light);

	m_Models.clear();
}

void frostwave::Scene::Init()
{
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

void frostwave::Scene::AddLight(DirectionalLight* light)
{
	m_DirectionalLights.push_back(light);
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
	for (auto* light : m_DirectionalLights)
		renderer->Submit(light);
}
