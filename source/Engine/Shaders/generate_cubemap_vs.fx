#include "generate_cubemap_include.fx"

VertexOutput VSMain(VertexInput input)
{
	float4 vertexObjectPos = input.position.xyzw;

    VertexOutput output;
    output.local_position = vertexObjectPos;
    output.position = mul(model, vertexObjectPos);
    return output;
}