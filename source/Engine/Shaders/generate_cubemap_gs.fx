#include "generate_cubemap_include.fx"

[maxvertexcount(18)]
void GSMain(triangle VertexOutput input[3], inout TriangleStream<GeometryOutput> cubemap_stream) 
{ 
    for(int f = 0; f < 6; ++f) 
    {
        GeometryOutput output;
        output.rt_index = f; 
        for(int v = 0; v < 3; ++v)
        { 
            output.position = mul(VP[f], input[v].position);
            output.local_position = input[v].local_position;
            cubemap_stream.Append(output);
        } 
        cubemap_stream.RestartStrip();
    } 
} 