
#include "StaticSamplers.hlsli"
#include "ShaderUtil.hlsli"
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"

cbuffer cbDoF : register(b0)
{
	float _Distance;
	float _LensCoeff;
	float _MaxCoC;
	float _RcpMaxCoC;
	float _RcpAspect;
};

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

// Input textures.
Texture2D gInputTexture					: register(t0);
Texture2D gCocTexture					: register(t1);

half4 main(VertexToPixel i) : SV_Target
{
	float3 duv = gInvRenderTargetSize.xyx * float3(0.5, 0.5, -0.5);
	float2 uv0 = i.uv.xy - duv.xy;
	float2 uv1 = i.uv.xy - duv.zy;
	float2 uv2 = i.uv.xy + duv.zy;
	float2 uv3 = i.uv.xy + duv.xy;

	// Sample source colors
	half3 c0 = gInputTexture.SampleLevel(linearClampSampler, uv0, 0.0f).rgb;
	half3 c1 = gInputTexture.SampleLevel(linearClampSampler, uv1, 0.0f).rgb;
	half3 c2 = gInputTexture.SampleLevel(linearClampSampler, uv2, 0.0f).rgb;
	half3 c3 = gInputTexture.SampleLevel(linearClampSampler, uv3, 0.0f).rgb;

	// Sample CoCs
	half coc0 = gCocTexture.SampleLevel(linearClampSampler, uv0, 0.0f).r * 2.0 - 1.0;
	half coc1 = gCocTexture.SampleLevel(linearClampSampler, uv1, 0.0f).r * 2.0 - 1.0;
	half coc2 = gCocTexture.SampleLevel(linearClampSampler, uv2, 0.0f).r * 2.0 - 1.0;
	half coc3 = gCocTexture.SampleLevel(linearClampSampler, uv3, 0.0f).r * 2.0 - 1.0;

	// Apply CoC and luma weights to reduce bleeding and flickering
	float w0 = abs(coc0) / (max(max(c0.r, c0.g), c0.b) + 1.0);
	float w1 = abs(coc1) / (max(max(c1.r, c1.g), c1.b) + 1.0);
	float w2 = abs(coc2) / (max(max(c2.r, c2.g), c2.b) + 1.0);
	float w3 = abs(coc3) / (max(max(c3.r, c3.g), c3.b) + 1.0);

	// Weighted average of the color samples
	half3 avg = c0 * w0 + c1 * w1 + c2 * w2 + c3 * w3;
	avg /= max(w0 + w1 + w2 + w3, 1e-5);

	// Select the largest CoC value
	half coc_min = min(coc0, min(min(coc1, coc2), coc3));
	half coc_max = max(coc0, max(max(coc1, coc2), coc3));
	half coc = (-coc_min > coc_max ? coc_min : coc_max) * _MaxCoC;

	// Premultiply CoC again
	avg *= smoothstep(0, gInvRenderTargetSize.y * 2, abs(coc));

	return half4(avg, coc);
}

