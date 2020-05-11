#include "fullscreen_include.fx"
#include "gaussian_include.fx"

Texture2D fullscreen_texture: register(t0);
SamplerState default_sampler : register(s0);

float4 PSMain(PixelInput input) : SV_TARGET
{
	float3 resource = fullscreen_texture.Sample(default_sampler, input.uv).rgb;

    float3 blur_color = float3(0,0,0);
    const uint kernel_size = 11;
    float start = ((kernel_size - 1) / 2.0) * -1.0;
    for (uint i = 0; i < kernel_size; ++i)
    {
        float2 uv = input.uv + float2(texel_size.x * (start + i), 0);
        float3 sampled = fullscreen_texture.Sample(default_sampler, uv).rgb;
        blur_color += sampled * GaussianKernel11[i];
    }

    return float4(blur_color, 1.0);
}