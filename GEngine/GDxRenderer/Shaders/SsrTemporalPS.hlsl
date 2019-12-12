
#include "StaticSamplers.hlsli"
#include "ShaderUtil.hlsli"
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"


#define USE_REFLECTION_VELOCITY 0

#define TEMPORAL_WEIGHT 0.99f
#define TEMPORAL_CLAMP_SCALE 1.25f
#define EXPOSURE 10.0f

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

struct PixelOutput
{
	half4	color : SV_TARGET0;
	half4	history : SV_TARGET1;
};

// Input textures.
Texture2D gInputTexture					: register(t0);
Texture2D gTraceTexture					: register(t1);
Texture2D gVelocityTexture				: register(t2);
Texture2D gHistoryTexture				: register(t3);

//--Temporal Filter Util.-----------------------------------------------------------------

float Luminance(float3 color)
{
	return (0.587f * color.r + 0.114f * color.g + 0.299f * color.b);
}

float3 ReconstructWorldPos(float2 uv, float depth)
{
	float ndcX = uv.x * 2 - 1;
	float ndcY = 1 - uv.y * 2; // Remember to flip y!!!
	float4 viewPos = mul(float4(ndcX, ndcY, depth, 1.0f), gUnjitteredInvProj);
	viewPos = viewPos / viewPos.w;
	return mul(viewPos, gInvView).xyz;
}

inline half2 CalculateMotion(float2 inUV, float rawDepth)
{
	float4 worldPos = float4(inUV, rawDepth, 1.0f);

	float4 prevClipPos = mul(worldPos, gPrevViewProj);
	float4 curClipPos = mul(worldPos, gUnjitteredViewProj);

	float2 prevHPos = prevClipPos.xy / prevClipPos.w;
	float2 curHPos = curClipPos.xy / curClipPos.w;

	// V is the viewport position at this pixel in the range 0 to 1.
	float2 vPosPrev = prevHPos.xy / float2(2.0f, -2.0f) + float2(0.5f, 0.5f);
	float2 vPosCur = curHPos.xy / float2(2.0f, -2.0f) + float2(0.5f, 0.5f);
	return vPosCur - vPosPrev;
}

inline void ResolveAABB
(
	Texture2D currColor,
	float ExposureScale,
	float AABBScale,
	float2 uv,
	float2 ScreenSize,
	inout half Variance,
	inout half4 MinColor,
	inout half4 MaxColor,
	inout half4 FilterColor
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

	float4 SampleColors[9];

	for (uint i = 0; i < 9; i++)
	{
		SampleColors[i] = currColor.Sample(basicSampler, uv + (SampleOffset[i] / ScreenSize));
	}

	half4 m1 = 0.0;
	half4 m2 = 0.0;
	for (uint x = 0; x < 9; x++)
	{
		m1 += SampleColors[x];
		m2 += SampleColors[x] * SampleColors[x];
	}

	half4 mean = m1 / 9.0;
	half4 stddev = sqrt((m2 / 9.0) - mean * mean);

	MinColor = mean - AABBScale * stddev;
	MaxColor = mean + AABBScale * stddev;

	FilterColor = SampleColors[4];
	MinColor = min(MinColor, FilterColor);
	MaxColor = max(MaxColor, FilterColor);

	/*
	half TotalVariance = 0;
	for (uint z = 0; z < 9; z++)
	{
		half temp = (Luminance(SampleColors[z]) - Luminance(mean));
		TotalVariance += temp * temp;
	}
	Variance = saturate((TotalVariance / 9) * 256);
	//Variance *= FilterColor.a;
	*/
}

PixelOutput main(VertexToPixel i)
{
	float2 UV = i.uv;

#if USE_REFLECTION_VELOCITY
	float hitZ = gTraceTexture.Sample(linearClampSampler, UV).b;
	float2 Velocity = CalculateMotion(UV, hitZ); // Reflection Depth derived motion. Removes smudghing cause by screen motion vectors.
#else
	float2 Velocity = gVelocityTexture.Sample(basicSampler, UV + gJitter).xy;
#endif

	// Get AABB ClipBox.
	half variance = 0;
	half4 currColor = 0;
	half4 minColor, maxColor;

	ResolveAABB(
		gInputTexture,
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
	half4 prevColor = gHistoryTexture.Sample(linearClampSampler, UV - Velocity);
	prevColor = clamp(prevColor, minColor, maxColor);

	// Combine.
	float temporalBlendWeight = saturate(TEMPORAL_WEIGHT * (1 - length(Velocity) * 8.0f));
	half4 color = lerp(currColor, prevColor, temporalBlendWeight);

	PixelOutput output;
	output.color = color;
	output.history = color;

	return output;
}

