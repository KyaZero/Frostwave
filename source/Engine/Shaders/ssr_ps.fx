#include "fullscreen_include.fx"

Texture2D screen_texture: register(t0);
Texture2D depth_texture: register(t1);
Texture2D normal_texture: register(t2);
Texture2D rmao_texture: register(t3);

Texture2D shadow_map : register(t8);
SamplerState default_sampler : register(s0);

float3 SSR(float3 position, float3 normal, float roughness)
{
    float3 reflection = reflect(position, normal);

    float VdotR = max(dot(normalize(position), normalize(reflection)), 0.0);
    float fresnel = pow(VdotR, 3);

    float3 step = reflection;
    float3 new_position = position + step;

    float loops = max(sign(VdotR), 0.0) * 30;
    [loop]
    for (int i = 0; i < loops; ++i)
    {
        float4 new_view_position = float4(new_position, 1.0);
        float4 sample_position = mul(projection, new_view_position);
        sample_position.xy = (sample_position.xy / sample_position.w) * float2(0.5, -0.5) + 0.5;

        float2 check_bounds_uv = max(sign(sample_position.xy * (1.0 - sample_position.xy)), 0.0);
        if (check_bounds_uv.x * check_bounds_uv.y < 1.0)
        {
            step *= 0.5;
            new_position -= step;
            continue;
        }

        float current_depth = abs(new_view_position.z / far_plane);
        float depth = depth_texture.Sample(default_sampler, sample_position.xy).r;
        float sample_depth = abs(GetNormalizedDepth(depth, inv_projection, far_plane).r);
        float delta = abs(current_depth - sample_depth);
        float bias = 0.001;
        //depth == 1 is skybox, which we want to reflect.
        if (delta < bias || depth == 1)
        {
            float2 reverted = (sample_position.xy);
            float2 fade_on_edges = reverted * 2.0 - 1.0;
            fade_on_edges = abs(fade_on_edges);
            float fade_amount = min(1.0 - fade_on_edges.x, 1.0 - fade_on_edges.y);
            return screen_texture.Sample(default_sampler, sample_position.xy).xyz * fresnel * fade_amount;
        }
   
        step *= 1.0 - 0.5 * max(sign(current_depth - sample_depth), 0.0);
        new_position += step * (sign(sample_depth - current_depth) + 0.0000001);
    }
    return float3(0,0,0);
}

float4 PSMain(PixelInput input) : SV_TARGET
{
    float depth = depth_texture.Sample(default_sampler, input.uv).r;
    float3 world_position = WorldPosFromDepth(depth, input.uv, inv_projection, inv_view);
    float3 normal = normal_texture.Sample(default_sampler, input.uv).xyz;
    float roughness = rmao_texture.Sample(default_sampler, input.uv).r;

    float3 view_position = mul(view, float4(world_position.xyz, 1.0)).xyz;
    float3 view_normal = mul(view, float4(normal.xyz, 0.0)).xyz;

    float3 color = SSR(view_position, normalize(view_normal), roughness) * pow((1.0 - roughness), 0.8);
    return float4(color, 1.0);
}