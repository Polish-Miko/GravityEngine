
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"


#define USE_FILTER 0

#define TEMPORAL_WEIGHT 0.99f

#define TEMPORAL_CLAMP_SCALE 1.25f

#define EXPOSURE 10.0f

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

Texture2D gInputTexture				: register(t0);
Texture2D gHistoryTexture			: register(t1);
Texture2D gVelocityBuffer			: register(t2);

SamplerState			basicSampler	: register(s0);
SamplerComparisonState	shadowSampler	: register(s1);

struct PixelOutput
{
	float	color						: SV_TARGET0;
	float	history						: SV_TARGET1;
};

//--Temporal Filter Util.-----------------------------------------------------------------

inline float Luma4
(
	float3 Color
)
{
	return (Color.g * 2) + (Color.r + Color.b);
}

inline float HdrWeight4
(
	float3 Color,
	float Exposure
)
{
	return rcp(Luma4(Color) * Exposure + 4);
}

inline float HdrWeight
(
	float Color,
	float Exposure
)
{
	return rcp(Color * Exposure + 4);
}

float Luminance(float color)
{
	return color;
}

float Luminance(float3 color)
{
	return Luma4(color);
}

inline void ResolveAABB
(
	Texture2D currColor,
	float ExposureScale,
	float AABBScale,
	float2 uv,
	float2 ScreenSize,
	inout float Variance,
	inout float MinColor,
	inout float MaxColor,
	inout float FilterColor
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
		SampleColors[i] = gInputTexture.Sample(basicSampler, uv + (SampleOffset[i] / ScreenSize)).r;
	}

#if USE_FILTER
	float SampleWeights[9];
	for (uint j = 0; j < 9; j++)
	{
		SampleWeights[j] = HdrWeight(SampleColors[j].r, ExposureScale);
	}

	float TotalWeight = 0;
	for (uint k = 0; k < 9; k++)
	{
		TotalWeight += SampleWeights[k];
	}
	SampleColors[4] = (SampleColors[0] * SampleWeights[0] +
		SampleColors[1] * SampleWeights[1] +
		SampleColors[2] * SampleWeights[2] +
		SampleColors[3] * SampleWeights[3] +
		SampleColors[4] * SampleWeights[4] +
		SampleColors[5] * SampleWeights[5] +
		SampleColors[6] * SampleWeights[6] +
		SampleColors[7] * SampleWeights[7] +
		SampleColors[8] * SampleWeights[8])
		/ TotalWeight;
#endif

	float m1 = 0.0;
	float m2 = 0.0;
	for (uint x = 0; x < 9; x++)
	{
		m1 += SampleColors[x];
		m2 += SampleColors[x] * SampleColors[x];
	}

	float mean = m1 / 9.0;
	float stddev = sqrt((m2 / 9.0) - mean * mean);

	MinColor = mean - AABBScale * stddev;
	MaxColor = mean + AABBScale * stddev;

	FilterColor = SampleColors[4];
	MinColor = min(MinColor, FilterColor);
	MaxColor = max(MaxColor, FilterColor);

	half TotalVariance = 0;
	for (uint z = 0; z < 9; z++)
	{
		half temp = (Luminance(SampleColors[z]) - Luminance(mean));
		TotalVariance += temp * temp;
	}
	Variance = saturate((TotalVariance / 9) * 256);
	//Variance *= FilterColor.a;
}

//********************************************************************************************************************************************************************
// Temporal filter.
//********************************************************************************************************************************************************************
PixelOutput main(VertexToPixel pIn)// : SV_Target
{
	float2 UV = pIn.uv;
	float2 Velocity = gVelocityBuffer.Sample(basicSampler, UV).xy;

	// Get AABB ClipBox.
	float variance = 0;
	float currColor = 0;
	float minColor, maxColor;

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
	float prevColor = gHistoryTexture.Sample(basicSampler, UV - Velocity).r;
	prevColor = clamp(prevColor, minColor, maxColor);

	// Combine.
	float temporalBlendWeight = saturate(TEMPORAL_WEIGHT * (1 - length(Velocity) * 2));
	float color = lerp(currColor, prevColor, temporalBlendWeight);

	PixelOutput output;
	output.color = color;
	output.history = color;

	return output;
}


