#include "fullscreen_include.fx"

Texture2D fullscreen_texture: register(t0);
Texture2D average_luminance_texture: register(t1);
SamplerState default_sampler : register(s0);

float3 Tonemap_Uchimura(float3 x, float3 P, float3 a, float3 m, float3 l, float3 c, float3 b) {
    // Uchimura 2017, "HDR theory and practice"
    // Math: https://www.desmos.com/calculator/gslcdxvipg
    // Source: https://www.slideshare.net/nikuque/hdr-theory-and-practicce-jp
    float3 l0 = ((P - m) * l) / a;
    float3 L0 = m - m / a;
    float3 L1 = m + (1.0 - m) / a;
    float3 S0 = m + l0;
    float3 S1 = m + a * l0;
    float3 C2 = (a * P) / (P - S1);
    float3 CP = -C2 / P;

    float3 w0 = 1.0 - smoothstep(0.0, m, x);
    float3 w2 = step(m + l0, x);
    float3 w1 = 1.0 - w0 - w2;

    float3 T = m * pow(x / m, c) + b;
    float3 S = P - (P - S1) * exp(CP * (x - S0));
    float3 L = m + a * (x - m);

    return T * w0 + L * w1 + S * w2;
}

float3 Tonemap_Uchimura(float3 x) {
    const float3 P = 1.0;  // max display brightness
    const float3 a = 1.0;  // contrast
    const float3 m = 0.22; // linear section start
    const float3 l = 0.4;  // linear section length
    const float3 c = 1.33; // black
    const float3 b = 0.0;  // pedestal
    return Tonemap_Uchimura(x, P, a, m, l, c, b);
}

static const float KeyValue = 0.115f;
float Log2Exposure(in float avgLuminance)
{
    float exposure = 0.0f;

    avgLuminance = max(avgLuminance, 0.00001f);
    float linearExposure = (KeyValue / avgLuminance);
    exposure = log2(max(linearExposure, 0.00001f));

    return exposure;
}

float LinearExposure(in float avgLuminance)
{
    return exp2(Log2Exposure(avgLuminance));
}

// Determines the color based on exposure settings
float3 CalcExposedColor(in float3 color, in float avgLuminance, in float offset, out float exposure)
{
    exposure = Log2Exposure(avgLuminance);
    exposure += offset;
    return exp2(exposure) * color;
}

float4 PSMain(PixelInput input) : SV_TARGET
{
	float3 hdr_color = fullscreen_texture.Sample(default_sampler, input.uv).rgb;

    float luminance = average_luminance_texture.Sample(default_sampler, float2(0.5, 0.5)).a;

    float e = 0;
    hdr_color = CalcExposedColor(hdr_color, luminance, 0, e);
    float3 ldr_color = Tonemap_Uchimura(hdr_color);
    ldr_color = LinearToGamma(ldr_color);
    return float4(ldr_color, 1.0);
}