#include "generate_cubemap_include.fx"

Texture2D equirectangular_map : register(t0);

static const float2 inv_atan = float2(0.1591, 0.3183);
float2 SampleSphericalMap(float3 v)
{
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= inv_atan;
    uv += 0.5;
    return uv;
}

SamplerState default_sampler : register(s0);

PixelOutput PSMain(GeometryOutput input)
{
    float2 uv = SampleSphericalMap(normalize(input.local_position.xyz));
    uv.y *= -1;
    uv.y += 1;

    float3 color = equirectangular_map.Sample(default_sampler, uv).rgb;

    PixelOutput output;
    output.color = float4(color, 1);
	return output;
}