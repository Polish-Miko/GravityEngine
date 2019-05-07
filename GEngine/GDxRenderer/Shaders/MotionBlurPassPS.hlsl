
#include "MainPassCB.hlsli"

//#define USE_MOTION_BLUR

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

// Input textures.
Texture2D gInputTexture				: register(t0);
Texture2D gVelocityBuffer			: register(t1);

SamplerState			basicSampler	: register(s0);
SamplerComparisonState	shadowSampler	: register(s1);

float4 main(VertexToPixel input) : SV_TARGET
{

#ifdef USE_MOTION_BLUR
	float2 velocitySampleUV = input.uv + gJitter;

	float2 velocity = gVelocityBuffer.Sample(basicSampler, velocitySampleUV).rg;

	float3 color = float3(0.0f, 0.0f, 0.0f);

	int numSamples = 9;
	float velScale[9] =
	{
		-0.7f,
		0.2f,
		0.5f,
		0.5f,
		0.4f,
		0.3f,
		0.2f,
		0.15f,
		0.1f
	};
	float2 sampleUV = input.uv;

	for (int i = 0; i < numSamples; ++i)
	{
		// Sample the color buffer along the velocity vector
		// Add the current color to our color sum
		color += gInputTexture.Sample(basicSampler, sampleUV).rgb;
		sampleUV = sampleUV + velocity * velScale[i];
	}
	// Average all of the samples to get the final blur color
	color = color / numSamples;
#else
	float3 color = gInputTexture.Sample(basicSampler, input.uv).rgb;
#endif

	return float4(color, 1.0f);

}

