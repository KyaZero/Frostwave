#pragma once
#include <Engine/Core/Math/Vec2.h>
#include <Engine/Core/Types.h>
#include <Engine/Core/InputHandler.h>
#include <functional>
#include <string>
#include <vector>

namespace frostwave
{
	struct WindowSettings
	{
		std::wstring title;
		u32 x, y, width, height;
		bool fullscreen;
	};

	using WindowHandle = void*;

	class Window
	{
	public:
		static Window* Get();
		struct Rect
		{
			i32 left;
			i32 top;
			i32 right;
			i32 bottom;
		};
		void AdjustWindowSize(Rect aRect);
		void UpdateTrueWindowSize();
		void Init(WindowSettings settings);
		void Run(std::function<void()> tick);
		void Subscribe(u32 message, std::function<void(u64, u32)> callback);
		void HandleMessage(u32 message, u64 wParam, u32 lParam);

		void Shutdown();
		bool ShouldRun();

		void ShowCursor(bool shouldShow);

		Vec2i GetBounds() const;
		Vec2f GetBoundsf() const;
		i32 GetWidth() const;
		i32 GetHeight() const;

		Vec2i GetPosition() const;
		i32 GetX() const;
		i32 GetY() const;

		f32 GetAspectRatio() const { return (f32)GetWidth() / (f32)GetHeight(); }

		bool GetFocused() const { return m_Focused; }

		WindowHandle GetHandle();
		InputHandler* GetInput();

		//TODO: Resize and stuff down here

	private:
		Window();
		~Window();

		struct Event
		{
			u32 message;
			std::function<void(u64, u32)> callback;
		};
		std::vector<Event> m_Events;
		WindowHandle m_Handle;
		WindowHandle m_Cursor;
		InputHandler m_Input;
		Vec2i m_Size, m_Position;
		bool m_ShouldRun, m_Focused, m_ShowCursor;
	};
}
namespace fw = frostwave;