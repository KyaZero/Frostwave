#include "fullscreen_include.fx"

Texture2D fullscreen_texture: register(t0);
SamplerState default_sampler : register(s0);

// Trims the algorithm from processing darks.
//   0.0833 - upper limit (default, the start of visible unfiltered edges)
//   0.0625 - high quality (faster)
//   0.0312 - visible limit (slower)
static const float contrast_threshold   = 0.0625;

// The minimum amount of local contrast required to apply algorithm.
//   0.333 - too little (faster)
//   0.250 - low quality
//   0.166 - default
//   0.125 - high quality 
//   0.063 - overkill (slower)
static const float relative_threshold   = 0.166;

// Choose the amount of sub-pixel aliasing removal.
// This can effect sharpness.
//   1.00 - upper limit (softer)
//   0.75 - default amount of filtering
//   0.50 - lower limit (sharper, less sub-pixel aliasing removal)
//   0.25 - almost off
//   0.00 - completely off
static const float subpixel_blending    = 0.75;

float4 Sample(float2 uv)
{
    return fullscreen_texture.Sample(default_sampler, uv);
}

float SampleLuminance(float2 uv)
{
    return Sample(uv).a;
}

float SampleLuminance(float2 uv, float u_offset, float v_offset)
{
    uv += texel_size * float2(u_offset, v_offset);
    return SampleLuminance(uv);
}

struct LuminanceData 
{
    float m, n, e, s, w;
    float ne, nw, se, sw;
    float highest, lowest, contrast;
};

LuminanceData SampleLuminanceNeighborHood(float2 uv)
{
    LuminanceData l;
    //middle
    l.m = SampleLuminance(uv);

    //north
    l.n = SampleLuminance(uv,  0,  1);
    //east
    l.e = SampleLuminance(uv,  1,  0);
    //south
    l.s = SampleLuminance(uv,  0, -1);
    //west
    l.w = SampleLuminance(uv, -1,  0);
    
    //north-east
    l.ne = SampleLuminance(uv,  1,  1);
    //north-west
    l.nw = SampleLuminance(uv, -1,  1);
    //south-east
    l.se = SampleLuminance(uv,  1, -1);
    //south-west
    l.sw = SampleLuminance(uv, -1, -1);

    l.highest = max(max(max(max(l.n, l.e), l.s), l.w), l.m);
    l.lowest = min(min(min(min(l.n, l.e), l.s), l.w), l.m);
    l.contrast = l.highest - l.lowest;

    return l;
}
    
bool ShouldSkipPixel(LuminanceData l)
{
    float threshold = max(contrast_threshold, relative_threshold * l.highest);
    return l.contrast < threshold;
}

float DeterminePixelBlendFactor(LuminanceData l)
{
    float filter = 2 * (l.n + l.e + l.s + l.w);
    filter += l.ne + l.nw + l.se + l.sw;
    filter *= 1.0 / 12;
    filter = abs(filter - l.m);
    filter = saturate(filter / l.contrast);
    float blend_factor = smoothstep(0, 1, filter);
    return blend_factor * blend_factor * subpixel_blending;
}

struct EdgeData 
{
    bool is_horizontal;
    float pixel_step;
    float opposite_luminance;
    float gradient;
};

EdgeData DetermineEdge(LuminanceData l)
{
    EdgeData e;
    float horizontal = 
        abs(l.n + l.s - 2 * l.m) * 2 +
        abs(l.ne + l.se - 2 * l.e) +
        abs(l.nw + l.sw - 2 * l.w);
    float vertical = 
        abs(l.e + l.w - 2 * l.m) * 2 + 
        abs(l.ne + l.nw - 2 * l.n) + 
        abs(l.se + l.sw - 2 * l.s);

    e.is_horizontal = horizontal >= vertical;

    float p_luminance = e.is_horizontal ? l.n : l.e;
    float n_luminance = e.is_horizontal ? l.s : l.w;

    float p_gradient = abs(p_luminance - l.m);
    float n_gradient = abs(n_luminance - l.m);

    e.pixel_step = e.is_horizontal ? texel_size.y : texel_size.x;

    if (p_gradient < n_gradient)
    {
        e.pixel_step = -e.pixel_step;
        e.opposite_luminance = n_luminance;
        e.gradient = n_gradient;
    }
    else
    {
        e.opposite_luminance = p_luminance;
        e.gradient = p_gradient;
    }

    return e;
}

#define EDGE_STEP_COUNT 10
#define EDGE_STEPS 1, 1.5, 2, 2, 2, 2, 2, 2, 2, 4
#define EDGE_GUESS 8

static const float edge_steps[EDGE_STEP_COUNT] = { EDGE_STEPS };

float DetermineEdgeBlendFactor(LuminanceData l, EdgeData e, float2 uv)
{
    float2 uv_edge = uv;
    float2 edge_step;
    if (e.is_horizontal)
    {
        uv_edge.y += e.pixel_step * 0.5;
        edge_step = float2(texel_size.x, 0);
    }
    else
    {
        uv_edge.x += e.pixel_step * 0.5;
        edge_step = float2(0, texel_size.y);
    }

    float edge_luminance = (l.m + e.opposite_luminance) * 0.5;
    float gradient_threshold = e.gradient * 0.25;

    float2 p_uv = uv_edge + edge_step * edge_steps[0];
    float p_luminance_delta = SampleLuminance(p_uv) - edge_luminance;
    float p_at_end = abs(p_luminance_delta) >= gradient_threshold;

    [unroll(EDGE_STEP_COUNT)]
    for(int i = 1; i < EDGE_STEP_COUNT && !p_at_end; ++i)
    {
        p_uv += edge_step * edge_steps[i];
        p_luminance_delta = SampleLuminance(p_uv) - edge_luminance;
        p_at_end = abs(p_luminance_delta) >= gradient_threshold;
    }
    
    if(!p_at_end)
        p_uv += edge_step * EDGE_GUESS;

    float2 n_uv = uv_edge - edge_step * edge_steps[0];
    float n_luminance_delta = SampleLuminance(n_uv) - edge_luminance;
    float n_at_end = abs(n_luminance_delta) >= gradient_threshold;

    [unroll(EDGE_STEP_COUNT)]
    for(int i = 1; i < EDGE_STEP_COUNT && !n_at_end; ++i)
    {
        n_uv -= edge_step * edge_steps[i];
        n_luminance_delta = SampleLuminance(n_uv) - edge_luminance;
        n_at_end = abs(n_luminance_delta) >= gradient_threshold;
    }

    if(!n_at_end)
        n_uv -= edge_step * EDGE_GUESS;

    float p_distance, n_distance;
    if (e.is_horizontal)
    {
        p_distance = p_uv.x - uv.x;
        n_distance = uv.x - n_uv.x;
    }
    else
    {
        p_distance = p_uv.y - uv.y;
        n_distance = uv.y - n_uv.y;
    }

    float shortest_distance;
    bool delta_sign;
    if (p_distance <= n_distance)
    {
        shortest_distance = p_distance;
        delta_sign = p_luminance_delta >= 0;
    }
    else
    {
        shortest_distance = n_distance;
        delta_sign = n_luminance_delta >= 0;
    }

    if(delta_sign == (l.m - edge_luminance >= 0))
        return 0;

    return 0.5 - shortest_distance / (p_distance + n_distance);
}

#define USE_FXAA 1

float4 PSMain(PixelInput input) : SV_TARGET
{
    float2 uv = input.uv;
    
#if USE_FXAA
    LuminanceData luminance_data = SampleLuminanceNeighborHood(uv);
    
    if (ShouldSkipPixel(luminance_data))
        discard;

    float pixel_blend = DeterminePixelBlendFactor(luminance_data);
    EdgeData edge_data = DetermineEdge(luminance_data);
    float edge_blend = DetermineEdgeBlendFactor(luminance_data, edge_data, uv);
    float final_blend = max(pixel_blend, edge_blend);

    if (edge_data.is_horizontal)
        uv.y += edge_data.pixel_step * final_blend;
    else
        uv.x += edge_data.pixel_step * final_blend;
#endif

    return float4(Sample(uv).rgb, 1);
}