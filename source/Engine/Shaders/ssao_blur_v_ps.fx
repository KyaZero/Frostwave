#include "fullscreen_include.fx"

Texture2D ssao_texture : register(t0);
Texture2D depth_texture : register(t1);

SamplerState default_sampler : register(s0);

static const float KERNEL_RADIUS = 7;
static const float SHARPNESS = 1000;

float4 BlurFunction(float2 uv, float r, float4 center_c, float center_d, inout float w_total)
{
    float4 c = ssao_texture.Sample(default_sampler, uv);
    float d = GetNormalizedDepth(depth_texture.Sample(default_sampler, uv).r, inv_projection, far_plane);

    const float blur_sigma = float(KERNEL_RADIUS) * 0.5;
    const float blur_falloff = 1.0 / (2.0 * blur_sigma*blur_sigma);

    float d_diff = (d - center_d) * SHARPNESS;
    float w = exp2(-r  * r * blur_falloff - d_diff * d_diff);
    w_total += w;

    return c * w;
}

float4 PSMain(PixelInput input) : SV_TARGET
{
    const float2 inv_resolution_direction = float2(0, 1.0 / float(size.y));

    float4 center_c = ssao_texture.Sample(default_sampler, input.uv);
    float center_d = GetNormalizedDepth(depth_texture.Sample(default_sampler, input.uv).r, inv_projection, far_plane);

    float4 c_total = center_c;
    float w_total = 1.0;

    for (float r = 1; r <= KERNEL_RADIUS; ++r)
    {
        float2 uv = input.uv + inv_resolution_direction * r;
        c_total += BlurFunction(uv, r, center_c, center_d, w_total);
    } 

    for (float r = 1; r <= KERNEL_RADIUS; ++r)
    {
        float2 uv = input.uv - inv_resolution_direction * r;
        c_total += BlurFunction(uv, r, center_c, center_d, w_total);
    } 

    return float4(c_total.rrr / w_total, 1.0);
}