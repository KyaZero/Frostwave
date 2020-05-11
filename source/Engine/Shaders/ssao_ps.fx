#include "fullscreen_include.fx"

Texture2D depth_texture : register(t0);
Texture2D normal_texture : register(t1);
Texture2D noise_texture : register(t15);

SamplerState default_sampler : register(s0);
SamplerState point_wrap_sampler : register(s3);

#define SSAO_RADIUS 1.5

float4 PSMain(PixelInput input) : SV_TARGET
{
    float depth = depth_texture.Sample(default_sampler, input.uv).r;
    float3 world_position = WorldPosFromDepth(depth, input.uv, inv_projection, inv_view);
    float3 frag_pos = world_position;
    float3 normal = normal_texture.Sample(default_sampler, input.uv).xyz;

    frag_pos = mul(view, float4(frag_pos.xyz, 1.0)).xyz;
    normal = mul(view, float4(normal.xyz, 0.0)).xyz;

    float2 noise_size = float2(8,8);
    float2 noise_scale = float2(size.x / noise_size.x, size.y / noise_size.y);
    float3 random_vec = noise_texture.Sample(point_wrap_sampler, input.uv * noise_scale).xyz;

    float3 tangent = normalize(random_vec - normal * dot(random_vec, normal));
    float3 bitangent = cross(tangent, normal);
    
    float3x3 tbn = float3x3(tangent, bitangent, normal);
    tbn = transpose(tbn);

    float occlusion = 0.0;
    for (int i = 0; i < 16; ++i)
    {
        float3 sample_pos = mul(tbn, kernel[i].xyz);
        sample_pos = frag_pos + sample_pos * SSAO_RADIUS;

        float4 offset = float4(sample_pos, 1.0);
        offset = mul(projection, offset);
        offset.xyz /= offset.w;
        
        float3 sample_dir = normalize(sample_pos - frag_pos);
        float NdotS = max(dot(normal, sample_dir), 0);

        float sample_depth = GetNormalizedDepth(depth_texture.Sample(default_sampler, float2(offset.x * 0.5 + 0.5, -offset.y * 0.5 + 0.5)), inv_projection, far_plane) * far_plane;

        float range_check = smoothstep(0,1,SSAO_RADIUS / abs(frag_pos.z - sample_depth));
        occlusion += range_check * step(sample_depth, sample_pos.z) * NdotS * 2.0;
    }
    occlusion = 1.0 - (occlusion / 16.0);

    return float4(occlusion.rrr, 1);
}