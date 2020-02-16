#pragma once
#include <Engine/Graphics/Camera.h>

class FreeCamera
{
public:
	FreeCamera(fw::Vec3f position);
	~FreeCamera();

	void Update(f32 dt);
	fw::Camera* GetRaw() { return m_Camera; }

private:
	fw::Camera* m_Camera;
	f32 m_Pitch, m_Yaw, m_Sensitivity, m_Speed, m_Modifier;
};