
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

half4 main(VertexToPixel i) : SV_Target
{
	// 9 tap tent filter with 4 bilinear samples
	const float4 duv = gInvRenderTargetSize.xyxy * 2.0f * float4(0.5f, 0.5f, -0.5f, 0.0f);
	half4 acc;
	acc = gInputTexture.SampleLevel(linearClampSampler, i.uv - duv.xy, 0.0f);
	acc += gInputTexture.SampleLevel(linearClampSampler, i.uv - duv.zy, 0.0f);
	acc += gInputTexture.SampleLevel(linearClampSampler, i.uv + duv.zy, 0.0f);
	acc += gInputTexture.SampleLevel(linearClampSampler, i.uv + duv.xy, 0.0f);
	return acc / 4.0;
}

