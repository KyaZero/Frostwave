cbuffer FrameBuffer
{
    float4x4 view;
    float4x4 proj;
    float4x4 invProj;
    float4x4 invView;
    float4 cameraPos;
    float nearZ;
    float farZ;
    float2 resolution;
}