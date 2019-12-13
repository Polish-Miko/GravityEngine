
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
Texture2D gDofTexture					: register(t0);
Texture2D gCocTexture					: register(t1);
Texture2D gDofInputTexture				: register(t2);

half4 main(VertexToPixel i) : SV_Target
{
	half4 dof = gDofTexture.SampleLevel(linearClampSampler, i.uv, 0.0f);
	half coc = gCocTexture.SampleLevel(linearClampSampler, i.uv, 0.0f).r;
	coc = (coc - 0.5) * 2.0 * _MaxCoC;

	// Convert CoC to far field alpha value.
	float ffa = smoothstep(gInvRenderTargetSize.y * 2.0, gInvRenderTargetSize.y * 4.0, coc);

	half4 color = gDofInputTexture.SampleLevel(linearClampSampler, i.uv, 0.0f);

	half alpha = max(max(dof.r, dof.g), dof.b);

	// lerp(lerp(color, dof, ffa), dof, dof.a)
	color = lerp(color, float4(dof.rgb, alpha), ffa + dof.a - ffa * dof.a);

	return color;
}

