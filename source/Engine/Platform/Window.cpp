#include "Window.h"
#include <Windows.h>
#include <Engine/Graphics/imgui/imgui.h>
#include <Engine/Graphics/imgui/imgui_impl_win32.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(handle, message, wParam, lParam))
		return 0;

	if (message == WM_CLOSE || message == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}

	auto window = (fw::Window*)GetWindowLongPtr(handle, GWLP_USERDATA);
	if (window != nullptr)
	{
		if (auto* input = window->GetInput())
		{
			if (input->HandleEvents(message, wParam, lParam))
			{
				return 0;
			}
		}
		window->HandleMessage((u32)message, (u64)wParam, (u32)lParam);
		if (!window->ShouldRun())
		{
			PostQuitMessage(0);
			return 0;
		}
		//if (windowHandler->GetInputHandler()->HandleEvents(message, wParam, lParam))
		//{
		//	return 0;
		//}
	}

	return DefWindowProc(handle, message, wParam, lParam);
}

frostwave::Window::Window() : m_Handle(nullptr), m_ShouldRun(true), m_Focused(true)
{

}

frostwave::Window::~Window()
{
	DestroyWindow((HWND)m_Handle);
	UnregisterClass(L"EngineWindow", GetModuleHandle(NULL));
}

frostwave::Window* frostwave::Window::Get()
{
	static Window window;
	return &window;
}

void frostwave::Window::Init(WindowSettings settings)
{
	WNDCLASS windowClass = { 0 };
	windowClass.lpszClassName = L"EngineWindow";
	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = WindowProc;
	RegisterClass(&windowClass);

	m_Handle = (WindowHandle)CreateWindow(windowClass.lpszClassName, settings.title.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP, settings.x, settings.y, settings.width, settings.height, nullptr, nullptr, nullptr, nullptr);
	ShowWindow((HWND)m_Handle, SW_SHOW);

	SetWindowLongPtr((HWND)m_Handle, GWLP_USERDATA, (LONG_PTR)this);

	if (m_Handle != nullptr)
	{
		DragAcceptFiles((HWND)m_Handle, TRUE);
		m_Size = { (i32)settings.width, (i32)settings.height };
		m_Position = { (i32)settings.x, (i32)settings.y };
		UpdateTrueWindowSize();
	}

	m_Cursor = LoadCursor(NULL, IDC_ARROW);
}

void frostwave::Window::Run(std::function<void()> tick)
{
	while (true)
	{
		MSG message = { 0 };
		while (PeekMessageW(&message, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessageW(&message);

			if (message.message == WM_QUIT)
			{
				return;
			}
		}
		tick();
		if (m_ShowCursor)
		{
			::SetCursor((HCURSOR)m_Cursor);
		}
		else
		{
			::SetCursor(NULL);
		}
		m_Input.Update();
	}
}

void frostwave::Window::Subscribe(u32 message, std::function<void(u64, u32)> callback)
{
	m_Events.push_back({ message, callback });
}

void frostwave::Window::HandleMessage(u32 message, u64 wParam, u32 lParam)
{
	if (message == WM_SETFOCUS) m_Focused = true;
	if (message == WM_KILLFOCUS) m_Focused = false;

	if (message == WM_SIZE)
	{
		UpdateTrueWindowSize();
	}

	if (message == WM_MOVE)
	{
		UpdateTrueWindowSize();
	}

	for (auto& event : m_Events)
	{
		if (event.message == message)
		{
			event.callback(wParam, lParam);
		}
	}
}

void frostwave::Window::Shutdown()
{
	m_ShouldRun = false;
}

bool frostwave::Window::ShouldRun()
{
	return m_ShouldRun;
}

frostwave::Vec2i frostwave::Window::GetBounds() const
{
	return Vec2i(GetWidth(), GetHeight());
}

frostwave::Vec2f frostwave::Window::GetBoundsf() const
{
	return Vec2f((f32)GetWidth(), (f32)GetHeight());
}

i32 frostwave::Window::GetWidth() const
{
	return m_Size.x;
}

i32 frostwave::Window::GetHeight() const
{
	return m_Size.y;
}

fw::Vec2i frostwave::Window::GetPosition() const
{
	return Vec2i(GetX(), GetY());
}

i32 frostwave::Window::GetX() const
{
	return m_Position.x;
}

i32 frostwave::Window::GetY() const
{
	return m_Position.y;
}

fw::WindowHandle frostwave::Window::GetHandle()
{
	return m_Handle;
}

frostwave::InputHandler* frostwave::Window::GetInput()
{
	return &m_Input;
}

void frostwave::Window::ShowCursor(bool shouldShow)
{
	m_ShowCursor = shouldShow;
}

void frostwave::Window::AdjustWindowSize(Rect rect)
{
	auto width = rect.right - rect.left;
	auto height = rect.bottom - rect.top;
	if (width % 2 != 0)
	{
		width += 1;
	}
	if (height % 2 != 0)
	{
		height += 1;
	}

	SetWindowPos((HWND)m_Handle, HWND_TOP, rect.left, rect.top, width, height, 0);
}

void frostwave::Window::UpdateTrueWindowSize()
{
	RECT client, rect;
	GetClientRect((HWND)m_Handle, &client);
	GetWindowRect((HWND)m_Handle, &rect);

	Rect r = { rect.left, rect.top, rect.right, rect.bottom };

	AdjustWindowSize(r);

	auto borderSize = abs((client.right - client.left) - (rect.right - rect.left)) / 2;
	auto borderTop = abs((client.bottom - client.top) - (rect.bottom - rect.top)) - borderSize;

	//auto width = rect.right - rect.left;
	auto clientWidth = client.right - client.left;
	//auto height = rect.bottom - rect.top;
	auto clientHeight = client.bottom - client.top;
	auto x = rect.left + borderSize;
	auto y = rect.top + borderTop;

	m_Size = { clientWidth, clientHeight };
	m_Position = { x, y };
}