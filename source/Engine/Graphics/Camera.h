#pragma once
#include <Engine/Core/Math/Mat4.h>
#include <Engine/Core/Math/Quat.h>

namespace frostwave
{
	class Camera
	{
	public:
		Camera();
		~Camera();

		void Init(f32 fov, f32 aspect, f32 nearZ, f32 farZ);
		void Update();

		const Mat4f& GetProjection() const;
		const Mat4f& GetView() const;

		void SetPosition(const Vec3f& position);
		const Vec3f& GetPosition() const;
		void SetRotation(const Quatf& rotation);
		const Quatf& GetRotation() const;

		f32 GetNearPlane();
		f32 GetFarPlane();

	private:
		Mat4f m_View, m_Projection;
		Vec3f m_Position;
		Quatf m_Rotation;
		f32 m_FOV, m_Near, m_Far;
	};
}
namespace fw = frostwave;