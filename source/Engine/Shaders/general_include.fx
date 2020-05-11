#include "common.fx"
#include "pbr_include.fx"
#include "vertex_include.fx"

struct PixelInput
{
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    float4 tangent : TANGENT;
    float4 bitangent : BITANGENT;
    float4 color : COLOR;
    float2 uv : UV;
};

struct PixelInputFullscreen
{
	float4 position : SV_POSITION;
	float2 uv : UV;
};

struct PixelOutput
{
    float4 color : SV_TARGET;
};

struct GBufferOutput
{
	float4 albedo   : SV_TARGET0;
	float4 normal   : SV_TARGET1;
	float4 rmao     : SV_TARGET2;
    float emissive  : SV_TARGET3;
};

cbuffer FrameBuffer : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float4x4 inv_proj;
    float4x4 inv_view;
    float4 camera_pos;
    float near_z;
    float far_z;
    float2 resolution;
};

struct Material
{
    float4 albedo;
    float metallic;
    float roughness;
    float ao;
    float emissive;
};

cbuffer ObjectBuffer : register(b1)
{
    float4x4 model;
    Material material;
}

struct PointLight
{
    float4 position;
    float4 color;
};

struct DirectionalLight
{
    float4 direction;
    float4 color;
};

cbuffer LightingBuffer : register(b2)
{
    float4x4 light_matrix;
    PointLight point_light;
    DirectionalLight directional_light;
}