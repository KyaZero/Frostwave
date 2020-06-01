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
	m_Sponza = fw::Allocate<fw::Model>("assets/meshes/sponza/Sponza.obj");
	m_Sponza->SetPosition(fw::Vec3f(0, -5, 0));
	m_Sponza->SetScale({ 0.02f, 0.02f, 0.02f });
	engine->GetScene()->AddModel(m_Sponza);

	m_Light = fw::Allocate<fw::DirectionalLight>();
	engine->GetScene()->AddLight(m_Light);

	m_Camera = fw::Allocate<FreeCamera>(fw::Vec3f(0, 0, 0));

	for (size_t y = 0; y < 7; y++)
	{
		for (size_t x = 0; x < 7; x++)
		{
			auto* sphere = fw::Model::GetSphere(0.33f, 32, 32);
			sphere->SetPosition({ 0, y - 3.5f, x - 3.5f });
			auto& sphereMat = sphere->GetMaterial();
			sphereMat.albedo = { 1, 1, 1, 1 };
			sphereMat.roughness = x / 7.0f;
			sphereMat.metallic = y / 7.0f;
			sphereMat.ao = 1;
			engine->GetScene()->AddModel(sphere);
		}
	}

	//for (i32 i = 0; i < 10; i++)
	//{
	//	auto* sphere = fw::Model::GetSphere(0.1f, 16, 16);
	//	sphere->GetMaterial().roughness = 1;
	//	sphere->GetMaterial().albedo = fw::Vec3f(fw::Rand(), fw::Rand(), fw::Rand());
	//	sphere->GetMaterial().emissive = 2.0f;
	//	sphere->SetPosition(fw::Vec3f(fw::RandomRange(-10.0f, 10.0f), fw::RandomRange(-5.0f, 5.0f), fw::RandomRange(-10.0f, 10.0f)));
	//	engine->GetScene()->AddModel(sphere);
	//}

	//for (i32 y = -1; y < 1; y++)
	//{
	//	for (i32 x = -1; x < 1; x++)
	//	{
	//		engine->GetScene()->AddLight(fw::Allocate<fw::PointLight>(fw::Vec3f(2, y * 5.0f + 2, x * 5.0f + 2), 50.0f, fw::Vec3f(1, 0, 1), 10.0f));
	//		auto* sphere = fw::Model::GetSphere(0.1f, 16, 16);
	//		sphere->GetMaterial().roughness = 1;
	//		sphere->GetMaterial().albedo = fw::Vec3f(1, 0, 1);
	//		sphere->GetMaterial().emissive = 2.0f;
	//		sphere->SetPosition(fw::Vec3f(2, y * 5.0f + 2, x * 5.0f + 2));
	//		engine->GetScene()->AddModel(sphere);
	//	}
	//}
}

void Game::Update(fw::Engine* engine, f32 dt)
{
	m_Camera->Update(dt);
	engine->GetScene()->SetCamera(m_Camera->GetRaw());

	if (m_Sponza)
	{
		ImGui::Begin("Sponza Material");

		auto& mat = m_Sponza->GetMaterial();

		static bool albo = false;

		if (ImGui::Checkbox("Albedo Override", &albo))
			mat.albedo = fw::Vec4f(1, 1, 1, 1);

		if (albo)
		{

			fw::Vec4f albedo = mat.albedo;
			ImGui::DragFloat3("Albedo", &albedo.x, 0.001f, 0.0f, 1.0f);
			mat.albedo = albedo;
		}
		else
		{
			mat.albedo.a = -1;
		}

		static bool aoo = false;

		if (ImGui::Checkbox("Ambient Occlusion Override", &aoo))
			mat.ao = 0;

		if (aoo)
		{

			f32 ao = mat.ao;
			ImGui::DragFloat("Ambient Occlusion", &ao, 0.001f, 0.0f, 1.0f);
			mat.ao = ao;
		}
		else
		{
			mat.ao = -1;
		}

		static bool eo = false;

		if (ImGui::Checkbox("Emissive Override", &eo))
			mat.emissive = 0;

		if (eo)
		{

			f32 emissive = mat.emissive;
			ImGui::DragFloat("Emissive", &emissive, 0.001f, 0.0f, 1.0f);
			mat.emissive = emissive;
		}
		else
		{
			mat.emissive = -1;
		}

		static bool ro = false;

		if (ImGui::Checkbox("Roughness Override", &ro))
			mat.roughness = 0;

		if (ro)
		{

			f32 roughness = mat.roughness;
			ImGui::DragFloat("Roughness", &roughness, 0.001f, 0.0f, 1.0f);
			mat.roughness = roughness;
		}
		else
		{
			mat.roughness = -1;
		}

		static bool mo = false;

		if (ImGui::Checkbox("Metallic Override", &mo))
			mat.metallic = 0;

		if (mo)
		{

			f32 metallic = mat.metallic;
			ImGui::DragFloat("Metallic", &metallic, 0.001f, 0.0f, 1.0f);
			mat.metallic = metallic;
		}
		else
		{
			mat.metallic = -1;
		}

		ImGui::End();
	}

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