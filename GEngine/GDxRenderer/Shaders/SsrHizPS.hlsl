
#include "StaticSamplers.hlsli"

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
	half4 minDepth = half4(
		gInputDepth.SampleLevel(linearClampSampler, i.uv, 0.0f, int2(-1.0,-1.0)).r,
		gInputDepth.SampleLevel(linearClampSampler, i.uv, 0.0f, int2(-1.0, 1.0)).r,
		gInputDepth.SampleLevel(linearClampSampler, i.uv, 0.0f, int2(1.0, -1.0)).r,
		gInputDepth.SampleLevel(linearClampSampler, i.uv, 0.0f, int2(1.0, 1.0)).r
	);

	return max(max(minDepth.r, minDepth.g), max(minDepth.b, minDepth.a));
}

