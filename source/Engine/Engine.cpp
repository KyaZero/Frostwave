#include "Engine.h"
#include <Engine/Graphics/RenderManager.h>
#include <Engine/Graphics/imgui/imgui.h>
#include <Engine/Platform/Window.h>
#include <Engine/Graphics/Shader.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Graphics/Scene.h>
#include <Engine/Core/Common.h>
#include <Engine/FileWatcher.h>
#include <filesystem>
#include <cassert>

//#undef WITH_EDITOR

frostwave::Engine::Engine(Size allocatedMemory)
{
	Allocator::Create(allocatedMemory);
	Logger::Create();
	Logger::SetLevel(Logger::Level::Info);

	m_RenderManager = Allocate();
	m_Scene = Allocate();
}

frostwave::Engine::~Engine()
{
	Free(m_Scene);
	Free(m_RenderManager);

	Logger::Destroy();
	Allocator::Destroy();
}

void frostwave::Engine::Init(std::function<void(f32)> gameUpdate, std::function<void()> gameInit, std::function<void(f32, const Texture*)> editorUpdate)
{
	WindowSettings settings = { };
	settings.width = 1280;
	settings.height = 720;
	settings.fullscreen = false;
	settings.title = L"Frostwave";
	Window::Get()->Init(settings);
	m_RenderManager->Init();

	gameInit();

	m_GameUpdate = gameUpdate;
	m_EditorUpdate = editorUpdate;
	Window::Get()->Run([&]() 
	{
		Tick(); 
	});
}

void frostwave::Engine::Tick()
{
	if (Window::Get()->GetInput()->IsKeyPressed(fw::Key::ESCAPE))
		Shutdown();

	m_RenderManager->BeginFrame();

	m_Timer.Update();
	f32 dt = m_Timer.GetDeltaTime();
	FileWatcher::Get()->Update(dt);

#ifdef _DEBUG
	m_DebugVisualizer.Draw();
#endif

#ifdef WITH_EDITOR
	ImGui::DockSpaceOverViewport();
#endif

	m_GameUpdate(dt);

	m_Scene->Submit(m_RenderManager);

#ifdef WITH_EDITOR
	m_RenderManager->Render(m_Timer.GetTotalTime(), m_Scene->GetCamera(), false);
	m_EditorUpdate(dt, m_RenderManager->GetRenderedScene());
#else
	m_RenderManager->Render(m_Timer.GetTotalTime(), m_Scene->GetCamera());
#endif
	m_RenderManager->EndFrame();
}

void frostwave::Engine::Shutdown()
{
	Window::Get()->Shutdown();
}
