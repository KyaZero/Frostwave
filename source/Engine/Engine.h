#pragma once
#include <Engine/Memory/Allocator.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Logging/Logger.h>
#include <Engine/Core/Timer.h>
#include <functional>

#ifdef _DEBUG
#include <Engine/Debug/DebugVisualizer.h>
#endif

namespace frostwave
{
	class Framework;
	class RenderManager;
	class Window;
	class Scene;
	class Engine
	{
	public:
		Engine(Size allocatedMemory);
		~Engine();
		void Init(std::function<void(f32)> gameUpdate, std::function<void()> gameInit, std::function<void(f32, const Texture*)> editorUpdate);
		void Tick();
		void Shutdown();

		Scene* GetScene() const { return m_Scene; }

	private:
		RenderManager* m_RenderManager;
		Texture* m_RenderedScene;
		Framework* m_Framework;
		Scene* m_Scene;
		Timer m_Timer;
#ifdef _DEBUG
		DebugVisualizer m_DebugVisualizer;
#endif
		std::function<void(f32)> m_GameUpdate;
		std::function<void(f32, const Texture*)> m_EditorUpdate;
	};
}
namespace fw = frostwave;