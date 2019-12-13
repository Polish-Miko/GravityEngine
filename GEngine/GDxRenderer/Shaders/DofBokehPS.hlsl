
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

static const int kSampleCount = 22;
static const float2 kDiskKernel[kSampleCount] = {
	float2(0,0),
	float2(0.53333336,0),
	float2(0.3325279,0.4169768),
	float2(-0.11867785,0.5199616),
	float2(-0.48051673,0.2314047),
	float2(-0.48051673,-0.23140468),
	float2(-0.11867763,-0.51996166),
	float2(0.33252785,-0.4169769),
	float2(1,0),
	float2(0.90096885,0.43388376),
	float2(0.6234898,0.7818315),
	float2(0.22252098,0.9749279),
	float2(-0.22252095,0.9749279),
	float2(-0.62349,0.7818314),
	float2(-0.90096885,0.43388382),
	float2(-1,0),
	float2(-0.90096885,-0.43388376),
	float2(-0.6234896,-0.7818316),
	float2(-0.22252055,-0.974928),
	float2(0.2225215,-0.9749278),
	float2(0.6234897,-0.7818316),
	float2(0.90096885,-0.43388376),
};

half4 main(VertexToPixel i) : SV_Target
{
	half4 samp0 = gInputDepth.SampleLevel(linearClampSampler, i.uv, 0.0f);

	half4 bgAcc = 0.0; // Background: far field bokeh
	half4 fgAcc = 0.0; // Foreground: near field bokeh

	[loop]
	for (int si = 0; si < kSampleCount; si++)
	{
		float2 disp = kDiskKernel[si] * _MaxCoC;
		float dist = length(disp);

		float2 duv = float2(disp.x * _RcpAspect, disp.y);
		half4 samp = gInputDepth.SampleLevel(linearClampSampler, i.uv + duv, 0.0f);

		// BG: Compare CoC of the current sample and the center sample
		// and select smaller one.
		half bgCoC = max(min(samp0.a, samp.a), 0.0);

		// Compare the CoC to the sample distance.
		// Add a small margin to smooth out.
		const half margin = gInvRenderTargetSize.y * 2;
		half bgWeight = saturate((bgCoC - dist + margin) / margin);
		half fgWeight = saturate((-samp.a - dist + margin) / margin);

		// Cut influence from focused areas because they're darkened by CoC
		// premultiplying. This is only needed for near field.
		fgWeight *= step(gInvRenderTargetSize.y, -samp.a);

		// Accumulation
		bgAcc += half4(samp.rgb, 1.0) * bgWeight;
		fgAcc += half4(samp.rgb, 1.0) * fgWeight;
	}

	// Get the weighted average.
	bgAcc.rgb /= bgAcc.a + (bgAcc.a == 0.0); // zero-div guard
	fgAcc.rgb /= fgAcc.a + (fgAcc.a == 0.0);

	// BG: Calculate the alpha value only based on the center CoC.
	// This is a rather aggressive approximation but provides stable results.
	bgAcc.a = smoothstep(gInvRenderTargetSize.y, gInvRenderTargetSize.y * 2.0, samp0.a);

	// FG: Normalize the total of the weights.
	fgAcc.a *= _PI / kSampleCount;

	// Alpha premultiplying
	half alpha = saturate(fgAcc.a);
	half3 rgb = lerp(bgAcc.rgb, fgAcc.rgb, alpha);

	return half4(rgb, alpha);
}

