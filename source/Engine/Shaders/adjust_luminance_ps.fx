#include "fullscreen_include.fx"

Texture2D current_luminance_texture : register(t0);
Texture2D last_luminance_texture : register(t1);
SamplerState default_sampler : register(s0);

float4 PSMain(PixelInput input) : SV_TARGET
{
    float current_luminance = current_luminance_texture.Sample(default_sampler, float2(0.5, 0.5)).a;
    float previous_luminance = last_luminance_texture.Sample(default_sampler, float2(0.5, 0.5)).a;
    float adaptation_factor = (current_luminance <= previous_luminance) ? /*brighten*/ 0.03 : /*darken*/ 0.03;
    float adapted_luminance = lerp(previous_luminance, current_luminance, adaptation_factor);
    return adapted_luminance;
}