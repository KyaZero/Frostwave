#include "fullscreen_include.fx"

PixelInput VSMain(uint index : SV_VertexID)
{
	float4 positions[3] =
	{
		float4(-1.0, -1.0, 0.0, 1.0),
		float4(-1.0,  3.0, 0.0, 1.0),
		float4( 3.0, -1.0, 0.0, 1.0)
	};

	float2 uvs[3] =
	{
		float2(0.0,  1.0),
		float2(0.0, -1.0),
		float2(2.0,  1.0)
	};

	PixelInput pixel_input;
	pixel_input.position = positions[index];
	pixel_input.uv = uvs[index];
	return pixel_input;
}
