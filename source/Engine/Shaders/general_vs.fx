#include "general_include.fx"

PixelInput VSMain(VertexInput input)
{
    PixelInput pixel_input;
    float4 object_pos = input.position;
    float4 world_pos = mul(model, object_pos);
    float4 view_pos = mul(view, world_pos);
    float4 proj_pos = mul(proj, view_pos);

    pixel_input.position = proj_pos;
    pixel_input.normal = input.normal;
    pixel_input.tangent = input.tangent;
    pixel_input.bitangent = input.bitangent;
    pixel_input.color = input.color;
    pixel_input.uv = input.uv;

    return pixel_input;
}