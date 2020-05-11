#include "general_include.fx"

Texture2D albedo_texture	: register(t1);
Texture2D normal_texture	: register(t2);
Texture2D roughness_texture	: register(t3);
Texture2D depth_texture	    : register(t5);

Texture2D shadow_map	    : register(t8);

SamplerState default_sampler : register(s1);

PixelOutput PSMain(PixelInputFullscreen input)
{
    float3 albedo = albedo_texture.Sample(default_sampler, input.uv).rgb;
    float roughness = roughness_texture.Sample(default_sampler, input.uv).r;
    float metallic = roughness_texture.Sample(default_sampler, input.uv).g;
    float depth = depth_texture.Sample(default_sampler, input.uv).r;
    float3 world_position = WorldPosFromDepth(depth, input.uv, inv_proj, inv_view);

    float3 N = normalize(normal_texture.Sample(default_sampler, input.uv).rgb);
    float3 V = normalize(camera_pos - world_position);

    float3 F0 = 0.04;
    F0 = lerp(F0, albedo, metallic);

    float3 Lo = 0;

    float3 L = directional_light.direction.xyz;
    float3 H = normalize(V + L);
    float3 radiance = directional_light.color.rgb * directional_light.color.a;

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

    float4 light_view = mul(light_matrix, float4(world_position, 1));
	float3 light_projection = light_view.xyz;
    float bias = 0.01;
	light_projection = light_projection * 0.5 + 0.5;
	light_projection.y = 1 - light_projection.y;

    float2 texmapscale = float2(1.0 / 2048, 1.0 / 2048) * 0.5;
    float shadow = 0;
    for(float y = -2; y < 2; y += 1.0)
    {
        for(float x = -2; x < 2; x += 1.0)
        {
            if(shadow_map.Sample(default_sampler, light_projection.xy + float2(x,y) * texmapscale).r < light_view.z - bias)
            {
                shadow += 1;
            }
        }
    }

    shadow /= 16;

    PixelOutput output;
    output.color = color * (1.0 - shadow);
    return output;
}