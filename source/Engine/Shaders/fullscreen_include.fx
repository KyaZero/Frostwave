#include "common.fx"

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : UV;
};

cbuffer FrameBuffer : register(b0)
{
	float4x4 view;
	float4x4 projection;
	float4x4 inv_projection;
	float4x4 inv_view;
	float4x4 light_matrix;
	float4 camera_pos;
	float4 light_direction;
	float4 light_color;
	float4 kernel[16];
	float2 resolution;
	float2 texel_size;
	float2 size;
	float far_plane;
	float near_plane;
}