#include "generate_cubemap_include.fx"

TextureCube environment_map : register(t0);
SamplerState default_sampler : register(s0);

static const float PI = 3.14159265359;

PixelOutput PSMain(GeometryOutput input)
{
    float3 normal = normalize(input.local_position);
    float3 irradiance = float3(0,0,0);

    float3 up = float3(0,1,0);
    float3 right = cross(up, normal);
    up = cross(normal, right);

    float sample_delta = 0.025;
    float nr_samples = 0.0;
    for(float phi = 0.0; phi < 2.0 * PI; phi += sample_delta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sample_delta)
        {
            float3 tangent_sample = float3(-(sin(theta) * cos(phi)), sin(theta) * sin(phi), cos(theta));
            float3 sample_vec = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * normal;
            sample_vec.x *= -1;
            irradiance += environment_map.Sample(default_sampler, sample_vec).rgb * cos(theta) * sin(theta);
            nr_samples++;
        }
    }

    irradiance = PI * irradiance * (1.0 / float(nr_samples));

    PixelOutput output;
    output.color = float4(irradiance, 1);
	return output;
}