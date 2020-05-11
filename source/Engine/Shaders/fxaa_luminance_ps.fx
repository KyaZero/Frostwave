#include "fullscreen_include.fx"

Texture2D fullscreen_texture: register(t0);
SamplerState default_sampler : register(s0);

// Convert rgb to luminance
// with rgb in linear space with sRGB primaries and D65 white point
float LinearRgbToLuminance(float3 linear_rgb) 
{
	return dot(linear_rgb, float3(0.2126729f,  0.7151522f, 0.0721750f));
}

float4 PSMain(PixelInput input) : SV_TARGET
{
	float4 resource = fullscreen_texture.Sample(default_sampler, input.uv).rgba;
    resource.a = LinearRgbToLuminance(saturate(resource));
    return resource;
}