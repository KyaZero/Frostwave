#include "FreeCamera.h"
#include <Engine/Platform/Window.h>
#include <Engine/Memory/Allocator.h>
#include <Engine/Core/Math/Quat.h>

FreeCamera::FreeCamera(fw::Vec3f position) : m_Sensitivity(100.0f), m_Modifier(1.0f), m_Speed(10.0f), m_Pitch(0), m_Yaw(0)
{
	m_Camera = fw::Allocate();
	m_Camera->Init(90.0f, fw::Window::Get()->GetAspectRatio(), 0.01f, 100.0f);
	m_Camera->SetPosition(position);
}

FreeCamera::~FreeCamera()
{
	fw::Free(m_Camera);
}

void FreeCamera::Update(f32 dt)
{
	auto window = fw::Window::Get();
	auto norm = window->GetInput()->GetMousePosition();
	auto mouseDelta = norm - fw::Vec2f(0.5f, 0.5f);
	if (abs(mouseDelta.x) < 0.001) mouseDelta.x = 0;
	if (abs(mouseDelta.y) < 0.001) mouseDelta.y = 0;
	static bool toggle = true;

	fw::Vec2f delta = (fw::Vec2f)mouseDelta;

	toggle = window->GetFocused() && window->GetInput()->IsMouseButtonDown(fw::MouseButton::Right);

	if (toggle)
	{
		window->GetInput()->SetMousePosition({0.5f, 0.5f});
		m_Pitch += delta.x * fw::Window::Get()->GetAspectRatio() * dt * m_Sensitivity;
		m_Yaw = fw::Clamp(m_Yaw + delta.y * dt * m_Sensitivity, -fw::PI / 2.0f, fw::PI / 2.0f);
	}
	window->ShowCursor(!toggle);

	fw::Quatf qp(fw::Vec3f(1, 0, 0), m_Yaw);
	fw::Quatf qy(fw::Vec3f(0, 1, 0), m_Pitch);

	fw::Quatf orientation = qp * qy;
	orientation.Normalize();

	if (window->GetInput()->IsKeyDown(fw::Key::SHIFT))
	{
		m_Modifier = 2.0f;
	}

	if (window->GetInput()->IsKeyDown(fw::Key::CONTROL))
	{
		m_Modifier = 0.1f;
	}

	auto pos = m_Camera->GetPosition();
	f32 speed = m_Speed * m_Modifier * dt;

	if (window->GetInput()->IsKeyDown(fw::Key::W))
		pos += orientation.GetForwardVector() * speed;

	if (window->GetInput()->IsKeyDown(fw::Key::S))
		pos -= orientation.GetForwardVector() * speed;

	if (window->GetInput()->IsKeyDown(fw::Key::D))
		pos += orientation.GetRightVector() * speed;

	if (window->GetInput()->IsKeyDown(fw::Key::A))
		pos -= orientation.GetRightVector() * speed;

	if (window->GetInput()->IsKeyDown(fw::Key::E))
		pos += fw::Vec3f(0,1,0) * speed;

	if (window->GetInput()->IsKeyDown(fw::Key::Q))
		pos -= fw::Vec3f(0, 1, 0) * speed;

	m_Modifier = 1.0f;

	m_Camera->SetPosition(pos);
	m_Camera->SetRotation(orientation);
	m_Camera->Update();
}