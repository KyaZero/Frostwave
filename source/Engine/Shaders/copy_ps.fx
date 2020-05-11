#include "fullscreen_include.fx"

Texture2D fullscreen_texture: register(t0);
SamplerState default_sampler : register(s0);

float4 PSMain(PixelInput input) : SV_TARGET
{
	float3 resource = fullscreen_texture.Sample(default_sampler, input.uv).rgb;
    float4 color = float4(resource, 1);
    return color;
}