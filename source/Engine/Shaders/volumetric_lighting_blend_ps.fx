#include "fullscreen_include.fx"

Texture2D source_texture: register(t0);
Texture2D volumetric_texture: register(t1);
SamplerState default_sampler : register(s0);

float4 PSMain(PixelInput input) : SV_TARGET
{
    float3 source = source_texture.Sample(default_sampler, input.uv).rgb;
    float3 volumetric = volumetric_texture.Sample(default_sampler, input.uv).rgb;

    return float4(source + volumetric, 1.0);
}