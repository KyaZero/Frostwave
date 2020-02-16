#include "InputHandler.h"
#include <Windows.h>
#include <Engine/Engine.h>
#include <Engine/Platform/Window.h>
#include <windowsx.h>

frostwave::InputHandler::InputHandler()
{
	for (size_t i = 0; i < 255; i++)
	{
		m_KeyState[i] = false;
		m_PreviousKeyState[i] = false;
	}
	for (size_t i = 0; i < 3; i++)
	{
		m_MouseState[i] = false;
		m_PreviousMouseState[i] = false;
	}
	m_MousePosition.x = 0;
	m_MousePosition.y = 0;

	m_DidPress = false;
}

frostwave::InputHandler::~InputHandler()
{
}

void frostwave::InputHandler::Init()
{
}

void frostwave::InputHandler::Update()
{
	m_DidPress = false;
	m_MouseWheelDelta = 0;
	m_PreviousMousePosition = m_MousePosition;

	for (size_t i = 0; i < 3; i++)
	{
		m_PreviousMouseState[i] = m_MouseState[i];
	}

	for (size_t i = 0; i < 255; i++)
	{
		m_PreviousKeyState[i] = m_KeyState[i];
	}
}

bool frostwave::InputHandler::HandleEvents(u32 message, i64 wParam, i64 lParam)
{
	switch (message)
	{
	case WM_MOUSEMOVE:
		m_MousePosition.x = GET_X_LPARAM(lParam);
		m_MousePosition.y = GET_Y_LPARAM(lParam);
		break;
	case WM_MOUSEWHEEL:
		m_MouseWheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		break;
	case WM_LBUTTONDOWN:
		m_MouseState[0] = true;
		break;
	case WM_LBUTTONUP:
		m_MouseState[0] = false;
		break;
	case WM_RBUTTONDOWN:
		m_MouseState[1] = true;
		break;
	case WM_RBUTTONUP:
		m_MouseState[1] = false;
		break;
	case WM_MBUTTONDOWN:
		m_MouseState[2] = true;
		break;
	case WM_MBUTTONUP:
		m_MouseState[2] = false;
		break;
	case WM_SYSKEYDOWN:
		m_KeyState[wParam] = true;
		break;
	case WM_SYSKEYUP:
		m_KeyState[wParam] = false;
		break;
	case WM_KEYDOWN:
		m_DidPress = true;
		m_LastPressed = (Key)wParam;
		m_KeyState[wParam] = true;
		break;
	case WM_KEYUP:
		m_KeyState[wParam] = false;
		break;
	default:
		return false;
		break;
	}

	return true;
}

bool frostwave::InputHandler::IsKeyDown(Key key) const
{
	return m_KeyState[static_cast<i32>(key)];
}

bool frostwave::InputHandler::IsKeyUp(Key key) const
{
	return !m_KeyState[static_cast<i32>(key)];
}

bool frostwave::InputHandler::IsKeyPressed(Key key) const
{
	return (m_KeyState[static_cast<i32>(key)] && !m_PreviousKeyState[static_cast<i32>(key)]);
}

bool frostwave::InputHandler::IsKeyReleased(Key key) const
{
	return (!m_KeyState[static_cast<i32>(key)] && m_PreviousKeyState[static_cast<i32>(key)]);
}

bool frostwave::InputHandler::IsMouseButtonDown(MouseButton button) const
{
	return m_MouseState[static_cast<i32>(button)];
}

bool frostwave::InputHandler::IsMouseButtonUp(MouseButton button) const
{
	return !m_MouseState[static_cast<i32>(button)];
}

bool frostwave::InputHandler::IsMouseButtonPressed(MouseButton button) const
{
	return (m_MouseState[static_cast<i32>(button)] && !m_PreviousMouseState[static_cast<i32>(button)]);
}

bool frostwave::InputHandler::IsMouseButtonReleased(MouseButton button) const
{
	return (!m_MouseState[static_cast<i32>(button)] && m_PreviousMouseState[static_cast<i32>(button)]);
}

i16 frostwave::InputHandler::GetMouseWheelDelta() const
{
	return m_MouseWheelDelta;
}

fw::Vec2i frostwave::InputHandler::GetMousePositionPixel() const
{
	return fw::Vec2i(m_MousePosition.x, m_MousePosition.y);
}

fw::Vec2f frostwave::InputHandler::GetMousePosition() const
{
	auto screen = Window::Get()->GetBounds();
	auto pos = GetMousePositionPixel();
	return fw::Vec2f((f32)m_MousePosition.x / (f32)screen.x, (f32)m_MousePosition.y / (f32)screen.y);
}

f32 frostwave::InputHandler::GetMouseAxis(const MouseAxis axis) const
{
	auto pos = GetMousePosition();
	if (axis == MouseAxis::X) return pos.x;
	return pos.y;
}

fw::Vec2f frostwave::InputHandler::GetPreviousMousePosition(fw::Vec2i screenSize) const
{
	return fw::Vec2f((f32)m_PreviousMousePosition.x / (f32)screenSize.x, (f32)m_PreviousMousePosition.y / (f32)screenSize.y);
}

fw::Vec2i frostwave::InputHandler::GetMousePositionDelta() const
{
	Vec2i result;
	result.x = m_MousePosition.x - m_PreviousMousePosition.x;
	result.y = m_MousePosition.y - m_PreviousMousePosition.y;
	return result;
}

fw::Vec2i frostwave::InputHandler::GetAbsoluteMousePosition() const
{
	POINT point;
	GetCursorPos(&point);
	Vec2i result;
	result.x = static_cast<short>(point.x);
	result.y = static_cast<short>(point.y);
	return result;
}

void frostwave::InputHandler::SetMousePosition(fw::Vec2f norm)
{
	auto winPos = Window::Get()->GetPosition();
	auto winSize = Window::Get()->GetBounds();
	SetMousePosition((i32)(norm.x * winSize.x + winPos.x), (i32)(norm.y * winSize.y + winPos.y));
}

void frostwave::InputHandler::SetMousePosition(i32 x, i32 y)
{
	SetCursorPos(x, y);
}

