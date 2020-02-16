#include "Game.h"
#include <Engine/Logging/Logger.h>
#include <Game/Component.h>
#include <Engine/Engine.h>
#include <entt/entt.hpp>
#include <Engine/Graphics/Scene.h>
#include <Engine/Graphics/imgui/imgui.h>
#include <Engine/Graphics/Lights.h>
#include <Engine/Core/Math/Quat.h>
#include <Engine/Platform/Window.h>

Game::Game()
{
}

Game::~Game()
{
	fw::Free(m_Camera);
}

void Game::Init(fw::Engine* engine)
{
	engine;
	fw::Model* sponza = fw::Allocate<fw::Model>("assets/meshes/sponza/sponzaPBR.obj");
	sponza->SetScale({ 0.01f, 0.01f, 0.01f });
	engine->GetScene()->AddModel(sponza);

	fw::Model* lux = fw::Allocate<fw::Model>("assets/meshes/lux/Lux_Mesh.obj");
	lux->SetScale({ 0.1f, 0.1f, 0.1f });
	engine->GetScene()->AddModel(lux);

	fw::Model* cerberus = fw::Allocate<fw::Model>("assets/meshes/Cerberus/Cerberus.fbx");
	cerberus->SetPosition({ 0, 1, 0 });
	cerberus->SetScale(0.01f);
	cerberus->SetRotation(fw::Quatf({ 1,0,0 }, -fw::PI / 2.0f));
	engine->GetScene()->AddModel(cerberus);

	engine->GetScene()->SetEnvironmentLight(fw::Allocate<fw::EnvironmentLight>(fw::Allocate<fw::Texture>("assets/cubemaps/panorama_cubemap.dds")));

	for (size_t i = 0; i < 16; i++)
	{
		//engine->GetScene()->AddLight(fw::Allocate<fw::PointLight>(fw::Vec3f(fw::Rand11() * 10.0f, fw::Rand() * 10.0f, fw::Rand11() * 10.0f),
		//	fw::Rand() * 5.0f, fw::Vec3f(fw::Rand(), fw::Rand(), fw::Rand()), fw::Rand() * 100.0f));
	}
	m_Light = fw::Allocate<fw::DirectionalLight>();
	engine->GetScene()->SetDirectionalLight(m_Light);

	m_Camera = fw::Allocate<FreeCamera>(fw::Vec3f(0, 0, 0));
}

void Game::Update(fw::Engine* engine, f32 dt)
{
	dt; engine;
	m_Camera->Update(dt);
	engine->GetScene()->SetCamera(m_Camera->GetRaw());

	if (m_Light)
	{
		ImGui::Begin("Directional Light", 0, ImGuiWindowFlags_AlwaysAutoResize);
		fw::Vec3f dir = m_Light->GetDirection();
		ImGui::DragFloat3("Direction", &dir.x, 0.01f);
		m_Light->SetDirection(dir);

		f32 intensity = m_Light->GetIntensity();
		ImGui::DragFloat("Intensity", &intensity, 0.01f);
		m_Light->SetIntensity(intensity);

		fw::Vec3f color = m_Light->GetColor();
		ImGui::DragFloat3("Color", &color.x, 0.001f);
		m_Light->SetColor(color);
		ImGui::End();
	}

}