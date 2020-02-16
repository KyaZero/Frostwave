#include "Camera.h"
#include <Engine/Core/Math/Mat3.h>
#include <Engine/Platform/Window.h>
#include <Windows.h>

frostwave::Camera::Camera() : m_FOV(0), m_Far(0), m_Near(0), m_Position(0, 0, 0), m_View(), m_Projection()
{
}

frostwave::Camera::~Camera()
{
}

void frostwave::Camera::Init(f32 fov, f32 aspect, f32 nearZ, f32 farZ)
{
	m_FOV = fov;
	m_Near = nearZ;
	m_Far = farZ;
	m_Projection = Mat4f::CreatePerspectiveProjection(m_FOV, aspect, m_Near, m_Far);

	Window::Get()->Subscribe(WM_SIZE, [&](auto, auto) {
		m_Projection = Mat4f::CreatePerspectiveProjection(m_FOV, (f32)Window::Get()->GetWidth() / (f32)Window::Get()->GetHeight(), m_Near, m_Far);
	});
}

void frostwave::Camera::Update()
{
	Mat4f transform = Mat4f::CreateTransform(m_Position, m_Rotation, { 1.0f, 1.0f, 1.0f });

	Mat3f transposed = transform;
	transposed = Mat3f::Transpose(transposed);
	Vec3f negated(-transform.m_Numbers[12], -transform.m_Numbers[13], -transform.m_Numbers[14]);
	negated *= transposed;
	
	m_View = {
		transposed.m_Numbers[0],	transposed.m_Numbers[1],	transposed.m_Numbers[2],	0,
		transposed.m_Numbers[3],	transposed.m_Numbers[4],	transposed.m_Numbers[5],	0,
		transposed.m_Numbers[6],	transposed.m_Numbers[7],	transposed.m_Numbers[8],	0,
		negated.x,					negated.y,					negated.z,					1
	};
}

const fw::Mat4f& frostwave::Camera::GetProjection() const
{
	return m_Projection;
}

const fw::Mat4f& frostwave::Camera::GetView() const
{
	return m_View;
}

void frostwave::Camera::SetPosition(const Vec3f& position)
{
	m_Position = position;
}

const fw::Vec3f& frostwave::Camera::GetPosition() const
{
	return m_Position;
}

void frostwave::Camera::SetRotation(const Quatf& rotation)
{
	m_Rotation = rotation;
}

const fw::Quatf& frostwave::Camera::GetRotation() const
{
	return m_Rotation;
}

f32 frostwave::Camera::GetNearPlane()
{
	return m_Near;
}

f32 frostwave::Camera::GetFarPlane()
{
	return m_Far;
}
