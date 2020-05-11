#include "fullscreen_include.fx"

Texture2D source_texture: register(t0);
Texture2D ssao_texture: register(t1);
SamplerState default_sampler : register(s0);

float4 PSMain(PixelInput input) : SV_TARGET
{
    float3 source = source_texture.Sample(default_sampler, input.uv).rgb;
    float ssao = ssao_texture.Sample(default_sampler, input.uv).r;

    return float4(source * ssao, 1.0);
}