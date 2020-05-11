struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 local_position : LOCAL_POSITION;
};

TextureCube environment_map : register(t0);
SamplerState default_sampler : register(s0);

float4 PSMain(VertexOutput input) : SV_TARGET
{
    float3 env_color = environment_map.SampleLevel(default_sampler, normalize(input.local_position.xyz), 0).rgb;
    return float4(env_color, 1.0);
}