#include "fullscreen_include.fx"

Texture2D source_texture : register(t0);
Texture2D luminance_texture : register(t1);
SamplerState default_sampler : register(s0);

float4 PSMain(PixelInput input) : SV_TARGET
{
	float3 source = source_texture.Sample(default_sampler, input.uv).rgb;
	float3 bloom = luminance_texture.Sample(default_sampler, input.uv).rgb;
    float4 color = float4(source + bloom, 1);
    return color;
}