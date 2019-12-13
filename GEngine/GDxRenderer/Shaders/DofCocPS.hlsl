
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
Texture2D gInputDepth					: register(t0);

half main(VertexToPixel i) : SV_Target
{
	float depth = ViewDepth(gInputDepth.SampleLevel(linearClampSampler, i.uv + gJitter, 0.0f).r);
	half coc = (depth - _Distance) * _LensCoeff / max(depth, 1e-5);
	return saturate(coc * 0.5 * _RcpMaxCoC + 0.5);
}

