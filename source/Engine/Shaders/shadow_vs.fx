#include "vertex_include.fx"

cbuffer FrameBuffer : register(b0)
{
	float4x4 vp;
}

cbuffer ObjectBuffer : register(b1)
{
    float4x4 model;
}

float4 VSMain(VertexInput input) : SV_POSITION
{
    return mul(vp, mul(model, input.position));
}