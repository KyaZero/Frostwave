#pragma once
#include <Engine/Core/Types.h>
#include <Engine/Core/Math/Vec3.h>

namespace frostwave
{
	struct Material
	{
		Vec4f albedo = Vec4f(1, 1, 1, -1);
		f32 metallic = -1.0f;
		f32 roughness = -1.0f;
		f32 ao = -1.0f;
		f32 emissive = -1.0f;
	};
}
namespace fw = frostwave;