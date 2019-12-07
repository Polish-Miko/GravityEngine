
#include "StaticSamplers.hlsli"
#include "ShaderUtil.hlsli"
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"


#define TEMPORAL_WEIGHT 0.99f
#define TEMPORAL_CLAMP_SCALE 1.25f
#define EXPOSURE 1.0f

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

struct PixelOutput
{
	half2	color : SV_TARGET0;
	half2	history : SV_TARGET1;
};

// Input textures.
Texture2D gOcclusionTexture				: register(t0);
Texture2D gDepthTexture					: register(t1);
Texture2D gVelocityTexture				: register(t2);
Texture2D gHistoryTexture				: register(t3);

//--Temporal Filter Util.-----------------------------------------------------------------

float Luminance(float2 color)
{
	return 0.5f * color.r + 0.5f * color.g;
}

inline void ResolveAABB
(
	Texture2D currColor,
	float ExposureScale,
	float AABBScale,
	float2 uv,
	float2 ScreenSize,
	inout half2 Variance,
	inout half2 MinColor,
	inout half2 MaxColor,
	inout half2 FilterColor
)
{
	const int2 SampleOffset[9] = {
		int2(-1.0, -1.0),
		int2(0.0, -1.0),
		int2(1.0, -1.0),
		int2(-1.0, 0.0),
		int2(0.0, 0.0),
		int2(1.0, 0.0),
		int2(-1.0, 1.0),
		int2(0.0, 1.0),
		int2(1.0, 1.0)
	};

	float SampleColors[9];

	for (uint i = 0; i < 9; i++)
	{
		SampleColors[i] = currColor.Sample(basicSampler, uv + (SampleOffset[i] / ScreenSize)).r;
	}

	half2 m1 = 0.0;
	half2 m2 = 0.0;
	for (uint x = 0; x < 9; x++)
	{
		m1 += SampleColors[x];
		m2 += SampleColors[x] * SampleColors[x];
	}

	half2 mean = m1 / 9.0;
	half2 stddev = sqrt((m2 / 9.0) - mean * mean);

	MinColor = mean - AABBScale * stddev;
	MaxColor = mean + AABBScale * stddev;

	FilterColor = SampleColors[4];
	MinColor = min(MinColor, FilterColor);
	MaxColor = max(MaxColor, FilterColor);

	half2 TotalVariance = 0;
	for (uint z = 0; z < 9; z++)
	{
		half temp = (Luminance(SampleColors[z]) - Luminance(mean));
		TotalVariance += temp * temp;
	}
	Variance = saturate((TotalVariance / 9) * 256);
	//Variance *= FilterColor.a;
}

PixelOutput main(VertexToPixel i)
{
	float2 UV = i.uv;
	float2 Velocity = gVelocityTexture.Sample(basicSampler, UV).xy;

	// Get AABB ClipBox.
	half2 variance = 0;
	half2 currColor = 0;
	half2 minColor, maxColor;

	ResolveAABB(
		gOcclusionTexture,
		EXPOSURE,
		TEMPORAL_CLAMP_SCALE,
		UV,
		gRenderTargetSize.xy,
		variance,
		minColor,
		maxColor,
		currColor
	);

	// Clamp.
	half2 prevColor = gHistoryTexture.Sample(basicSampler, UV - Velocity).rg;
	prevColor = clamp(prevColor, minColor, maxColor);

	// Combine.
	float temporalBlendWeight = saturate(TEMPORAL_WEIGHT * (1 - length(Velocity) * 2));
	half2 color = lerp(currColor, prevColor, temporalBlendWeight);

	PixelOutput output;
	output.color = color;
	output.history = color;

	return output;
}

