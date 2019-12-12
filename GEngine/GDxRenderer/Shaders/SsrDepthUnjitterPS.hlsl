
#include "StaticSamplers.hlsli"
#include "MainPassCB.hlsli"

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

// Input textures.
Texture2D gInputDepth					: register(t0);

// Velocity texture setup
half main(VertexToPixel i) : SV_Target
{
	return gInputDepth.SampleLevel(linearClampSampler, i.uv + gJitter, 0.0f).r;
}

