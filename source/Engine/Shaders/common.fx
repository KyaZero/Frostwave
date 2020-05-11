float3 GammaToLinear(float3 gamma_color)
{
    return pow(gamma_color, 2.2);
}

float3 LinearToGamma(float3 linear_color)
{
    return pow(linear_color, 1.0 / 2.2);
}

float3 WorldPosFromDepth(float depth, float2 uv, float4x4 inv_proj, float4x4 inv_view)
{
    float z = depth;
    float4 clip_space_position = float4(uv * 2.0 - 1.0, z, 1.0) * float4(1,-1,1,1);
    float4 view_space_position = mul(inv_proj, clip_space_position);

    view_space_position /= view_space_position.w;

    float4 world_space_position = mul(inv_view, view_space_position);
    return world_space_position.xyz;
}

float3 GetNormalizedDepth(float non_linear_depth, float4x4 inv_projection, float far_plane)
{
    float4 ndc_coords = float4(0,0,non_linear_depth,1.0);
    float4 view_coords = mul(inv_projection, ndc_coords);
    float linear_depth = view_coords.z / view_coords.w;
    float normalized_depth = linear_depth / far_plane;
    return normalized_depth;
}

float LinearizeDepth(float depth, float near_plane, float far_plane)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
}