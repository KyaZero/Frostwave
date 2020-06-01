#include "general_include.fx"

Texture2D albedo_texture	    : register (t0); 
Texture2D normal_texture	    : register (t1);
Texture2D metallic_texture		: register (t2); 
Texture2D roughness_texture     : register (t3);
Texture2D ambient_texture       : register (t4);
Texture2D emissive_texture      : register (t5);

SamplerState default_sampler : register(s1);

GBufferOutput PSMain(PixelInput input)
{
    float3 albedo_map = GammaToLinear(albedo_texture.Sample(default_sampler, input.uv).rgb);
    if (length(albedo_map) > 0 && albedo_texture.SampleLevel(default_sampler, input.uv, 0).a < 1) 
        discard;
    float3 normal_map = normal_texture.Sample(default_sampler, input.uv).rgb;
    float roughness_map = roughness_texture.Sample(default_sampler, input.uv).r;
    float metallic_map = metallic_texture.Sample(default_sampler, input.uv).r;
    float ambient_map = ambient_texture.Sample(default_sampler, input.uv).r;
    float emissive_map = emissive_texture.Sample(default_sampler, input.uv).r;

    float3 normal = input.normal.xyz;
    float3 albedo = material.albedo.rgb;
    float roughness = material.roughness;
    float metallic = material.metallic;
    float ambient = material.ao;
    float emissive = material.emissive;

    if(length(normal_map) > 0)
    {
        //transform normal map
        float3x3 tbn = float3x3(input.tangent.xyz, input.bitangent.xyz, input.normal.xyz);
        tbn = transpose(tbn);
        float3 n = normal_map;
        n = normalize((n * 2) - 1);
        normal = mul(tbn, n).xyz;
    }

    if (material.albedo.a < 0)
        albedo = albedo_map;

    if (material.roughness < 0)
        roughness = roughness_map;

    if(material.metallic < 0)
        metallic = metallic_map;

    if(material.ao < 0)
        ambient = ambient_map;

    if(material.emissive < 0)
        emissive = emissive_map;

    GBufferOutput gbuffer;
    gbuffer.albedo      = float4(albedo.rgb, 1);
    gbuffer.normal      = float4(normal, 1);
    gbuffer.rmao        = float4(roughness, metallic, ambient, 1);
    gbuffer.emissive    = emissive;
    return gbuffer;
}