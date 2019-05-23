
#include "ShaderDefinition.h"
//#include "ObjectCB.hlsli"
#include "MainPassCB.hlsli"
//#include "HaltonSequence.hlsli"

//#define SPATIAL_WEIGHT_CATMULLROM

//#define USE_FILTER

//#define USE_NEIGHBORHOOD_SPATIAL_WEIGHT
//#define USE_FIXED_NEIGHBORHOOD_SPATIAL_WEIGHT 1

#define USE_YCOCG

#define USE_TONEMAP

#define CENTER_TO_JITTER 0.0f

#define UST_CLOSEST_VELOCITY

//#define TEST

#define CLIP_TO_CENTER

//#define USE_HIGH_PASS_FILTER
//#define HIGH_FREQUENCY_SCALE 0.1f

//#define USE_MIXED_TONE_MAP
//#define MIXED_TONE_MAP_LINEAR_UPPER_BOUND 0.5f

static const float VarianceClipGamma = 1.0f;
static const float Exposure = 10;
static const float BlendWeightLowerBound = 0.03f;
static const float BlendWeightUpperBound = 0.12f;
static const float BlendWeightVelocityScale = 100.0f * 60.0f;


struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

//Light Render Results
Texture2D gInputTexture				: register(t0);
Texture2D gHistoryTexture			: register(t1);
Texture2D gVelocityBuffer			: register(t2);
Texture2D gDepthBuffer				: register(t3);
//Texture2D<uint2> gStencilBuffer			: register(t4);

SamplerState			basicSampler	: register(s0);
SamplerComparisonState	shadowSampler	: register(s1);

struct PixelOutput
{
	float4	color						: SV_TARGET0;
	float4	history						: SV_TARGET1;
};

static const float SampleOffsets[9][2] =
{
	{ -1.0f, -1.0f },
	{  0.0f, -1.0f },
	{  1.0f, -1.0f },
	{ -1.0f,  0.0f },
	{  0.0f,  0.0f },
	{  1.0f,  0.0f },
	{ -1.0f,  1.0f },
	{  0.0f,  1.0f },
	{  1.0f,  1.0f },
};

static const float neighborhoodFixedSpatialWeight[9] =
{
	1.0f / 16.0f,
	1.0f / 8.0f,
	1.0f / 16.0f,
	1.0f / 8.0f,
	1.0f / 4.0f,
	1.0f / 8.0f,
	1.0f / 16.0f,
	1.0f / 8.0f,
	1.0f / 16.0f
};

float LinearDepth(float depth)
{
	return (depth * NEAR_Z) / (FAR_Z - depth * (FAR_Z - NEAR_Z));
}

static float CatmullRom(float x)
{
	float ax = abs(x);
	if (ax > 1.0f)
		return ((-0.5f * ax + 2.5f) * ax - 4.0f) *ax + 2.0f;
	else
		return (1.5f * ax - 2.5f) * ax*ax + 1.0f;
}

float3 RGB2YCoCgR(float3 rgbColor)
{
	float3 YCoCgRColor;

	YCoCgRColor.y = rgbColor.r - rgbColor.b;
	float temp = rgbColor.b + YCoCgRColor.y / 2;
	YCoCgRColor.z = rgbColor.g - temp;
	YCoCgRColor.x = temp + YCoCgRColor.z / 2;

	return YCoCgRColor;
}

float3 YCoCgR2RGB(float3 YCoCgRColor)
{
	float3 rgbColor;

	float temp = YCoCgRColor.x - YCoCgRColor.z / 2;
	rgbColor.g = YCoCgRColor.z + temp;
	rgbColor.b = temp - YCoCgRColor.y / 2;
	rgbColor.r = rgbColor.b + YCoCgRColor.y;

	return rgbColor;
}

float Luminance(in float3 color)
{
#ifdef USE_TONEMAP
	return color.r;
#else
	return dot(color, float3(0.25f, 0.50f, 0.25f));
#endif
}

float3 ToneMap(float3 color)
{
#ifdef USE_MIXED_TONE_MAP
	float luma = Luminance(color);
	if (luma <= MIXED_TONE_MAP_LINEAR_UPPER_BOUND)
	{
		return color;
	}
	else
	{
		return color * (MIXED_TONE_MAP_LINEAR_UPPER_BOUND * MIXED_TONE_MAP_LINEAR_UPPER_BOUND - luma) / (luma*(2 * MIXED_TONE_MAP_LINEAR_UPPER_BOUND - 1 - luma));
	}
#else
	return color / (1 + Luminance(color));
#endif
}

float3 UnToneMap(float3 color)
{
#ifdef USE_MIXED_TONE_MAP
	float luma = Luminance(color);
	if (luma <= MIXED_TONE_MAP_LINEAR_UPPER_BOUND)
	{
		return color;
	}
	else
	{
		return color * (MIXED_TONE_MAP_LINEAR_UPPER_BOUND * MIXED_TONE_MAP_LINEAR_UPPER_BOUND - (2 * MIXED_TONE_MAP_LINEAR_UPPER_BOUND - 1) * luma) / (luma*(1 - luma));
	}
#else
	return color / (1 - Luminance(color));
#endif
}

// Faster but less accurate luma computation. 
// Luma includes a scaling by 4.
float Luma4(float3 Color)
{
#ifdef USE_TONEMAP
	return Color.r;
#else
	return (Color.g * 2.0) + (Color.r + Color.b);
#endif
}

// Optimized HDR weighting function.
float HdrWeight4(float3 Color, float Exposure)
{
	return rcp(Luma4(Color) * Exposure + 4.0);
}

float3 ClipAABB(float3 aabbMin, float3 aabbMax, float3 prevSample, float3 avg)
{
#ifdef CLIP_TO_CENTER
	// note: only clips towards aabb center (but fast!)
	float3 p_clip = 0.5 * (aabbMax + aabbMin);
	float3 e_clip = 0.5 * (aabbMax - aabbMin);

	float3 v_clip = prevSample - p_clip;
	float3 v_unit = v_clip.xyz / e_clip;
	float3 a_unit = abs(v_unit);
	float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

	if (ma_unit > 1.0)
		return p_clip + v_clip / ma_unit;
	else
		return prevSample;// point inside aabb
#else
	float3 r = prevSample - avg;
	float3 rmax = aabbMax - avg.xyz;
	float3 rmin = aabbMin - avg.xyz;

	const float eps = 0.000001f;

	if (r.x > rmax.x + eps)
		r *= (rmax.x / r.x);
	if (r.y > rmax.y + eps)
		r *= (rmax.y / r.y);
	if (r.z > rmax.z + eps)
		r *= (rmax.z / r.z);

	if (r.x < rmin.x - eps)
		r *= (rmin.x / r.x);
	if (r.y < rmin.y - eps)
		r *= (rmin.y / r.y);
	if (r.z < rmin.z - eps)
		r *= (rmin.z / r.z);

	return avg + r;
#endif
}

PixelOutput main(VertexToPixel pIn)// : SV_TARGET
{
	int x, y, i;
	float2 velocity;
	float lenVelocity;

	PixelOutput output;

	double2 JitteredUV = (double2)pIn.uv + double2(gJitter.x, gJitter.y);

#ifdef USE_CLOSEST_VELOCITY
	float2 closestOffset = float2(0.0f, 0.0f);
	float closestDepth = FAR_Z;
	for (y = -1; y <= 1; ++y)
	{
		for (x = -1; x <= 1; ++x)
		{
			float2 sampleOffset = float2(x, y) * gInvRenderTargetSize;
			float2 sampleUV = JitteredUV + sampleOffset;
			sampleUV = saturate(sampleUV);

			float NeighborhoodDepthSamp = gDepthBuffer.Sample(basicSampler, sampleUV).r;
			NeighborhoodDepthSamp = LinearDepth(NeighborhoodDepthSamp);
#if USE_REVERSE_Z
			if (NeighborhoodDepthSamp > closestDepth)
#else
			if (NeighborhoodDepthSamp < closestDepth)
#endif
			{
				closestDepth = NeighborhoodDepthSamp;
				closestOffset = float2(x, y);
			}
		}
	}
	closestOffset *= gInvRenderTargetSize;
	velocity = gVelocityBuffer.Sample(basicSampler, JitteredUV + closestOffset).rg;
#else
	velocity = gVelocityBuffer.Sample(basicSampler, JitteredUV).rg;
#endif
	lenVelocity = length(velocity);

	float2 InputSampleUV = lerp(pIn.uv, JitteredUV, CENTER_TO_JITTER);
	float3 currColor = gInputTexture.Sample(basicSampler, InputSampleUV).rgb;

#ifdef USE_TONEMAP
	currColor = ToneMap(currColor);
#endif

#ifdef USE_YCOCG
	currColor = RGB2YCoCgR(currColor);
#endif

	float3 prevColor = gHistoryTexture.Sample(basicSampler, pIn.uv - velocity).rgb;

#ifdef USE_TONEMAP
	prevColor = ToneMap(prevColor);
#endif

#ifdef USE_YCOCG
	prevColor = RGB2YCoCgR(prevColor);
#endif

	if (gFrameCount == 0)
	{
		output.color = float4(currColor, 1.0f);
		output.history = float4(currColor, 1.0f);
		return output;
	}

	// Init sampler kernel.

#ifdef USE_NEIGHBORHOOD_SPATIAL_WEIGHT
	float neighborhoodSpatialWeight[9];
	float TotalSpatialWeight = 0.0f;
	for (i = 0; i < 9; i++)
	{
		float PixelOffsetX = SampleOffsets[i][0] + gJitter.x;
		float PixelOffsetY = SampleOffsets[i][1] + gJitter.y;

#ifdef SPATIAL_WEIGHT_CATMULLROM
		neighborhoodSpatialWeight[i] = CatmullRom(PixelOffsetX) * CatmullRom(PixelOffsetY);
		TotalSpatialWeight += neighborhoodSpatialWeight[i];
#else
		// Normal distribution, Sigma = 0.47
		neighborhoodSpatialWeight[i] = exp(-2.29f * (PixelOffsetX * PixelOffsetX + PixelOffsetY * PixelOffsetY));
		TotalSpatialWeight += neighborhoodSpatialWeight[i];
#endif
	}

	for (i = 0; i < 9; i++)
	{
		neighborhoodSpatialWeight[i] = neighborhoodSpatialWeight[i] / TotalSpatialWeight;
	}
#endif

	// Sample neighborhoods.

	uint N = 9;
	float TotalWeight = 0.0f;
	float3 sum = 0.0f;
	float3 m1 = 0.0f;
	float3 m2 = 0.0f;
	float3 neighborMin = float3(9999999.0f, 9999999.0f, 9999999.0f);
	float3 neighborMax = float3(-99999999.0f, -99999999.0f, -99999999.0f);
	float3 neighborhood[9];
	float neighborhoodHdrWeight[9];
	float neighborhoodFinalWeight = 0.0f;

	for (y = -1; y <= 1; ++y)
	{
		for (x = -1; x <= 1; ++x)
		{
			i = (y + 1) * 3 + x + 1;
			float2 sampleOffset = float2(x, y) * gInvRenderTargetSize;
			float2 sampleUV = InputSampleUV + sampleOffset;
			sampleUV = saturate(sampleUV);

			float3 NeighborhoodSamp = gInputTexture.Sample(basicSampler, sampleUV).rgb;
			NeighborhoodSamp = max(NeighborhoodSamp, 0.0f);
#ifdef USE_TONEMAP
			NeighborhoodSamp = ToneMap(NeighborhoodSamp);
#endif
#ifdef USE_YCOCG
			NeighborhoodSamp = RGB2YCoCgR(NeighborhoodSamp);
#endif

			neighborhood[i] = NeighborhoodSamp;
			neighborhoodHdrWeight[i] = HdrWeight4(NeighborhoodSamp, Exposure);
			neighborMin = min(neighborMin, NeighborhoodSamp);
			neighborMax = max(neighborMax, NeighborhoodSamp);

#ifdef USE_NEIGHBORHOOD_SPATIAL_WEIGHT
	#if USE_FIXED_NEIGHBORHOOD_SPATIAL_WEIGHT == 1
			neighborhoodFinalWeight = neighborhoodHdrWeight[i] * neighborhoodFixedSpatialWeight[i];
	#else
			neighborhoodFinalWeight = neighborhoodHdrWeight[i] * neighborhoodSpatialWeight[i];
	#endif
#else 
			neighborhoodFinalWeight = neighborhoodHdrWeight[i];
#endif
			m1 += NeighborhoodSamp;
			m2 += NeighborhoodSamp * NeighborhoodSamp;
			TotalWeight += neighborhoodFinalWeight;
			sum += neighborhood[i] * neighborhoodFinalWeight;
		}
	}
	float3 Filtered = sum / TotalWeight;

#ifdef USE_HIGH_PASS_FILTER

	float3 highFreq = neighborhood[1] + neighborhood[3] + neighborhood[5] + neighborhood[7] - 4 * neighborhood[4];
	float3 sharpen;

#ifdef USE_FILTER
	sharpen = Filtered;
#else
	sharpen = currColor;
#endif

	sharpen += highFreq * HIGH_FREQUENCY_SCALE;

#ifdef USE_TONE_MAP
	saturate(sharpen);
#else
	sharpen = max(0.0f, sharpen);
#endif

#ifdef USE_FILTER
	Filtered = sharpen;
#else
	currColor = sharpen;
#endif

#endif

	// Variance clip.
	float3 mu = m1 / N;
	float3 sigma = sqrt(abs(m2 / N - mu * mu));
	float3 minc = mu - VarianceClipGamma * sigma;
	float3 maxc = mu + VarianceClipGamma * sigma;

	//prevColor = ClipAABB(neighborMin, neighborMax, prevColor, (neighborMin + neighborMax) / 2);
	prevColor = ClipAABB(minc, maxc, prevColor, mu);

	float weightCurr = lerp(BlendWeightLowerBound, BlendWeightUpperBound, saturate(lenVelocity * BlendWeightVelocityScale));
	float weightPrev = 1.0f - weightCurr;

	float RcpWeight = rcp(weightCurr + weightPrev);

#ifdef USE_FILTER
	float3 color = (Filtered * weightCurr + prevColor * weightPrev) * RcpWeight;
#else
	float3 color = (currColor * weightCurr + prevColor * weightPrev) * RcpWeight;
#endif

#ifdef USE_YCOCG
	color = YCoCgR2RGB(color);
#endif

#ifdef USE_TONEMAP
	color = UnToneMap(color);
#endif

	output.color = float4(color, 1.0f);
	output.history = float4(color, 1.0f);

#ifdef TEST

	float3 test = gInputTexture.Sample(basicSampler, pIn.uv).rgb;
	//test = (test * gNearZ) / (gFarZ - test * (gFarZ - gNearZ));
	output.color = float4(test, 1.0f);

#endif

	return output;

}

