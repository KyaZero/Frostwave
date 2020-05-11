#include "general_include.fx"

Texture2D albedo_texture	: register(t1);
Texture2D normal_texture	: register(t2);
Texture2D roughness_texture	: register(t3);
Texture2D depth_texture	    : register(t5);

SamplerState default_sampler : register(s1);

PixelOutput PSMain(PixelInput input, float4 fragCoord : SV_POSITION)
{
	float2 uv = fragCoord.xy / (float2)resolution.xy;
    float3 albedo = albedo_texture.Sample(default_sampler, uv).rgb;
    float roughness = roughness_texture.Sample(default_sampler, uv).r;
    float metallic = roughness_texture.Sample(default_sampler, uv).g;
    float depth = depth_texture.Sample(default_sampler, uv).r;
    float3 world_position = WorldPosFromDepth(depth, uv, inv_proj, inv_view);

    float3 N = normalize(normal_texture.Sample(default_sampler, uv).rgb);
    float3 V = normalize(camera_pos - world_position);

    float3 F0 = 0.04;
    F0 = lerp(F0, albedo, metallic);

    float3 Lo = 0;

    float3 L = normalize(point_light.position.xyz - world_position);
    float3 H = normalize(V + L);
    float distance = length(point_light.position.xyz - world_position);
    float attenuation = 1.0 / (distance * distance);
    float3 radiance = point_light.color.rgb * attenuation * point_light.color.a;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    float3 specular = numerator / max(denominator, 0.001);

    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;

    float4 color = float4(Lo, 1);

    PixelOutput output;
    output.color = color;
    return output;
}