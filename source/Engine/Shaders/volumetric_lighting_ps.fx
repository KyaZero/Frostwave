#include "fullscreen_include.fx"

Texture2D depth_texture: register(t0);
Texture2D shadow_map : register(t8);
SamplerState default_sampler : register(s0);

#define G_SCATTERING 0.85
#define G_SKYBOX_SCATTERING 0.95
#define NB_STEPS 30
#define PI 3.1415

#define iterBayerMat 1
#define bayer2x2(a) (4-(a).x-((a).y<<1))%4

float GetBayerFromCoordLevel(float2 pixelpos)
{
    int2 p = int2(pixelpos);
    int a = 0;
    for(int i = 0; i < iterBayerMat; i++)
    {
        a += bayer2x2(p>>(iterBayerMat-1-i)&1)<<(2*i);
    }
    return float(a)/float(2<<(iterBayerMat*2-1));
}

float bayer2  (float2 a){a=floor(a);return frac(dot(a,float2(.5, a.y*.75)));}
float bayer4  (float2 a){return bayer2 (  .5*a)*.25    +bayer2(a);}
float bayer8  (float2 a){return bayer4 (  .5*a)*.25    +bayer2(a);}
float bayer16 (float2 a){return bayer4 ( .25*a)*.0625  +bayer4(a);}
float bayer32 (float2 a){return bayer8 ( .25*a)*.0625  +bayer4(a);}
float bayer64 (float2 a){return bayer8 (.125*a)*.015625+bayer8(a);}
float bayer128(float2 a){return bayer16(.125*a)*.015625+bayer8(a);}
#define dither2(p)   (bayer2(  p)-.375      )
#define dither4(p)   (bayer4(  p)-.46875    )
#define dither8(p)   (bayer8(  p)-.4921875  )
#define dither16(p)  (bayer16( p)-.498046875)
#define dither32(p)  (bayer32( p)-.499511719)
#define dither64(p)  (bayer64( p)-.49987793 )
#define dither128(p) (bayer128(p)-.499969482)

float iib(float2 u){
    return dither128(u);
}
static float2 poisson_disk[64] = {
	float2(-0.613392, 0.617481),
	float2(0.170019, -0.040254),
	float2(-0.299417, 0.791925),
	float2(0.645680, 0.493210),
	float2(-0.651784, 0.717887),
	float2(0.421003, 0.027070),
	float2(-0.817194, -0.271096),
	float2(-0.705374, -0.668203),
	float2(0.977050, -0.108615),
	float2(0.063326, 0.142369),
	float2(0.203528, 0.214331),
	float2(-0.667531, 0.326090),
	float2(-0.098422, -0.295755),
	float2(-0.885922, 0.215369),
	float2(0.566637, 0.605213),
	float2(0.039766, -0.396100),
	float2(0.751946, 0.453352),
	float2(0.078707, -0.715323),
	float2(-0.075838, -0.529344),
	float2(0.724479, -0.580798),
	float2(0.222999, -0.215125),
	float2(-0.467574, -0.405438),
	float2(-0.248268, -0.814753),
	float2(0.354411, -0.887570),
	float2(0.175817, 0.382366),
	float2(0.487472, -0.063082),
	float2(-0.084078, 0.898312),
	float2(0.488876, -0.783441),
	float2(0.470016, 0.217933),
	float2(-0.696890, -0.549791),
	float2(-0.149693, 0.605762),
	float2(0.034211, 0.979980),
	float2(0.503098, -0.308878),
	float2(-0.016205, -0.872921),
	float2(0.385784, -0.393902),
	float2(-0.146886, -0.859249),
	float2(0.643361, 0.164098),
	float2(0.634388, -0.049471),
	float2(-0.688894, 0.007843),
	float2(0.464034, -0.188818),
	float2(-0.440840, 0.137486),
	float2(0.364483, 0.511704),
	float2(0.034028, 0.325968),
	float2(0.099094, -0.308023),
	float2(0.693960, -0.366253),
	float2(0.678884, -0.204688),
	float2(0.001801, 0.780328),
	float2(0.145177, -0.898984),
	float2(0.062655, -0.611866),
	float2(0.315226, -0.604297),
	float2(-0.780145, 0.486251),
	float2(-0.371868, 0.882138),
	float2(0.200476, 0.494430),
	float2(-0.494552, -0.711051),
	float2(0.612476, 0.705252),
	float2(-0.578845, -0.768792),
	float2(-0.772454, -0.090976),
	float2(0.504440, 0.372295),
	float2(0.155736, 0.065157),
	float2(0.391522, 0.849605),
	float2(-0.620106, -0.328104),
	float2(0.789239, -0.419965),
	float2(-0.545396, 0.538133),
	float2(-0.178564, -0.596057)
};

float DoShadowCheap(Texture2D shadowMap, float4 lightPos)
{
	float3 lightProjection = lightPos.xyz;
	lightProjection = lightProjection * 0.5 + 0.5;
	lightProjection.y = 1 - lightProjection.y;

	float shadow = 1.0;
    float bias = 0.00001;
	float samples = 0;
	const int nsamples = 4;
	for (int i = 0; i < nsamples; i++)
	{
		int idx = i;
		if (shadowMap.Sample(default_sampler, lightProjection.xy + poisson_disk[idx] / 2048.0).r < lightPos.z - bias)
		{
			samples += 1;
		}
	}
	
	shadow = 1 - saturate(samples / nsamples);
    if(lightProjection.z > 1.0)
        shadow = 0.0;
	return shadow;
}

// Mie scaterring approximated with Henyey-Greenstein phase function.
float ComputeScattering(float LdotV)
{
    float result = 1.0 - G_SCATTERING * G_SCATTERING;
    result /= (4.0 * PI * pow(1.0 + G_SCATTERING * G_SCATTERING - (2.0 * G_SCATTERING) * LdotV, 1.5));
    return result;
}

float4 PSMain(PixelInput input) : SV_TARGET
{
    float depth = depth_texture.Sample(default_sampler, input.uv).r;
    float3 world_position = WorldPosFromDepth(depth, input.uv, inv_projection, inv_view);
    float3 start_position = camera_pos;
    float3 end_ray_position = world_position;
    float steps = NB_STEPS;

    float3 ray_vector = end_ray_position - start_position;

    float ray_length = length(ray_vector);
    float3 ray_direction = ray_vector / ray_length;

    float3 step_length = ray_length / steps;

    float3 step = ray_direction * step_length;
    start_position += step * iib(input.uv * size);
    
    float3 current_position = start_position;

    float3 accum_fog = 0.0;

    float d = dot(ray_direction, normalize(light_direction.xyz));

    for(int i = 0; i < steps; ++i)
    {
        float4 light_position = mul(light_matrix, float4(current_position, 1));
        accum_fog += ComputeScattering(d) * DoShadowCheap(shadow_map, light_position);
        current_position += step;
    }
    accum_fog /= steps;

    float4 color = 1;
    color.rgb = accum_fog * (light_color.xyz * light_color.a);
    return color;
}