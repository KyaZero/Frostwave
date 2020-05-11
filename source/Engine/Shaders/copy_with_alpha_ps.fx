#include "fullscreen_include.fx"

Texture2D fullscreen_texture: register(t0);
SamplerState default_sampler : register(s0);

float4 PSMain(PixelInput input) : SV_TARGET
{
    return fullscreen_texture.Sample(default_sampler, input.uv);
}