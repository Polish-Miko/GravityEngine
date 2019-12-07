
#include "StaticSamplers.hlsli"
#include "ShaderUtil.hlsli"
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"

#define BLUR_RADIUS 12
#define SHARPNESS 0.2f

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

// Input textures.
Texture2D gOcclusionTexture				: register(t0);
Texture2D gDepthTexture					: register(t1);
Texture2D gVelocityTexture				: register(t2); 

inline void GetAo_Depth(float2 uv, inout float2 AO_RO, inout float AO_Depth)
{
	float2 SSAOTexture = gOcclusionTexture.SampleLevel(linearClampSampler, uv, 0.0f).rg;
	float SSAODepth = ViewDepth(gDepthTexture.SampleLevel(linearClampSampler, uv, 0.0f).r);
	AO_RO = SSAOTexture.xy;
	AO_Depth = SSAODepth;
}

inline float CrossBilateralWeight(float r, float d, float d0)
{
	const float BlurSigma = (float)BLUR_RADIUS * 0.5;
	const float BlurFalloff = 1 / (2 * BlurSigma * BlurSigma);

	float dz = (d0 - d) * Z_UPPER_BOUND * SHARPNESS / 100.0f;
	return exp2(-r * r * BlurFalloff - dz * dz);
}

inline void ProcessSample(float3 AO_RO_Depth, float r, float d0, inout float2 totalAOR, inout float totalW)
{
	float w = CrossBilateralWeight(r, d0, AO_RO_Depth.z);
	totalW += w;
	totalAOR += w * AO_RO_Depth.xy;
}

inline void ProcessRadius(float2 uv0, float2 deltaUV, float d0, inout float2 totalAO_RO, inout float totalW)
{
	float r = 1.0;
	float z = 0.0;
	float2 uv = 0.0;
	float2 AO_RO = 0.0;

	[unroll]
	for (; r <= BLUR_RADIUS / 2; r += 1) 
	{
		uv = uv0 + r * deltaUV;
		GetAo_Depth(uv, AO_RO, z);
		ProcessSample(float3(AO_RO, z), r, d0, totalAO_RO, totalW);
	}

	[unroll]
	for (; r <= BLUR_RADIUS; r += 2) 
	{
		uv = uv0 + (r + 0.5) * deltaUV;
		GetAo_Depth(uv, AO_RO, z);
		ProcessSample(float3(AO_RO, z), r, d0, totalAO_RO, totalW);
	}
}

inline float3 BilateralBlur(float2 uv0, float2 deltaUV)
{
	float depth;
	float2 totalAOR;
	GetAo_Depth(uv0, totalAOR, depth);
	float totalW = 1;

	ProcessRadius(uv0, -deltaUV, depth, totalAOR, totalW);
	ProcessRadius(uv0, deltaUV, depth, totalAOR, totalW);

	totalAOR /= totalW;
	return float3(totalAOR, depth);
}

half2 main(VertexToPixel i) : SV_Target
{
	half2 uv = i.uv;
	half3 AOR = BilateralBlur(uv, half2(gInvRenderTargetSize.x, 0));
	return AOR.rg;
}

