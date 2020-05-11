struct GeometryOutput
{
    float4 position : SV_POSITION;
    float4 local_position : LOCAL_POSITION;
    uint rt_index : SV_RenderTargetArrayIndex;
};

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

struct PixelOutput
{
	float4 color : SV_TARGET;
};

cbuffer FrameBuffer : register(b0)
{
    float4x4 VP[6];
    float roughness;
};

struct Material
{
    float4 albedo;
    float metallic;
    float roughness;
    float ao;
    float _; //padding
};

cbuffer ObjectBuffer : register(b1)
{
    float4x4 model;
    Material material;
}