struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 local_position : LOCAL_POSITION;
};

struct VertexInput
{
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 tangent : TANGENT;
    float4 bitangent : BITANGENT;
    float4 color : COLOR;
    float2 uv : UV;
};

cbuffer FrameBuffer : register(b0)
{
    float4x4 view;
    float4x4 proj;
};

VertexOutput VSMain(VertexInput input)
{
    float3x3 rot_view = (float3x3)view;
    float4 view_position = float4(mul(rot_view, input.position.xyz), 1.0);
    float4 clip_pos = mul(proj, view_position);

    VertexOutput output;
    output.local_position = input.position.xyzw;
    output.position = clip_pos.xyww;
    return output;
}