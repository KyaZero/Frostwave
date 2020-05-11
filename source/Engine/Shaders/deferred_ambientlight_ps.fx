#include "general_include.fx"

Texture2D albedo_texture	: register(t1);
Texture2D normal_texture	: register(t2);
Texture2D rmao_texture	    : register(t3);
Texture2D emissive_texture	: register(t4);
Texture2D depth_texture	    : register(t5);

TextureCube irradiance_map  : register(t14);
TextureCube prefiltered_map : register(t15);
Texture2D brdf_map          : register(t16);

SamplerState default_sampler: register(s1);

float4 PSMain(PixelInputFullscreen input) : SV_TARGET
{
    float3 albedo = albedo_texture.Sample(default_sampler, input.uv).rgb;
    float roughness = rmao_texture.Sample(default_sampler, input.uv).r;
    float metallic = rmao_texture.Sample(default_sampler, input.uv).g;
    float ao = rmao_texture.Sample(default_sampler, input.uv).b;
    float3 emissive = albedo * emissive_texture.Sample(default_sampler, input.uv).rrr;
    float depth = depth_texture.Sample(default_sampler, input.uv).r;
    float3 world_position = WorldPosFromDepth(depth, input.uv, inv_proj, inv_view);

    float3 N = normalize(normal_texture.Sample(default_sampler, input.uv).rgb);
    float3 V = normalize(camera_pos - world_position);
    
    float3 F0 = 0.04;
    F0 = lerp(F0, albedo, metallic);

    float3 R = reflect(-V, N);

    const float MAX_REFLECTION_LOD = 4.0;
    float3 prefiltered_color = prefiltered_map.SampleLevel(default_sampler, R, roughness * MAX_REFLECTION_LOD).rgb;

    float3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    float2 env_BRDF = brdf_map.Sample(default_sampler, float2(max(dot(N, V), 0.0), roughness)).rg;
    float3 specular = prefiltered_color * (F * env_BRDF.x + env_BRDF.y);

    float3 irradiance = irradiance_map.Sample(default_sampler, N).rgb;
    float3 diffuse    = irradiance * albedo;
    float3 ambient    = (kD * diffuse + specular) * ao; 

    kD = saturate(kD);

    return float4(ambient + emissive, 1);
}