
//#include "ObjectCB.hlsli"
#include "MainPassCB.hlsli"
#include "HaltonSequence.hlsli"

//#define ANTI_FLICKERING

static const float VarianceClipGamma = 1.0f;
static const float ExposureScale = 10;


struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

//Light Render Results
Texture2D gInputTexture				: register(t0);
Texture2D gHistoryTexture			: register(t1);
Texture2D gVelocityBuffer			: register(t2);

SamplerState			basicSampler	: register(s0);
SamplerComparisonState	shadowSampler	: register(s1);

struct PixelOutput
{
	float4	color						: SV_TARGET0;
	float4	history						: SV_TARGET1;
};

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
	return dot(color, float3(0.25f, 0.50f, 0.25f));
}

float3 ToneMap(float3 color)
{
	return color / (1 + Luminance(color));
}

float3 UnToneMap(float3 color)
{
	return color / (1 - Luminance(color));
}

// Faster but less accurate luma computation. 
// Luma includes a scaling by 4.
float Luma4(float3 Color)
{
	return (Color.g * 2.0) + (Color.r + Color.b);
}

// Optimized HDR weighting function.
float HdrWeight4(float3 Color, float Exposure)
{
	return rcp(Luma4(Color) * Exposure + 4.0);
}

float3 ClipAABB(float3 aabbMin, float3 aabbMax, float3 prevSample, float3 avg)
{
#if 1
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
#else
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
#endif
}

PixelOutput main(VertexToPixel pIn)// : SV_TARGET
{

	PixelOutput output;

	uint subsampIndex = gFrameCount % SAMPLE_COUNT;
	float2 Jitter = float2(
		Halton_2_3[subsampIndex].x * gInvRenderTargetSize.x * JitterDistance,
		Halton_2_3[subsampIndex].y * gInvRenderTargetSize.y * JitterDistance
		) / 2;
	float2 JitteredUV = pIn.uv - Jitter;

	float2 velocity = gVelocityBuffer.Sample(basicSampler, JitteredUV).rg;

	float3 currColor = gInputTexture.Sample(basicSampler, JitteredUV).rgb;
	currColor = ToneMap(currColor);
	currColor = RGB2YCoCgR(currColor);
	float3 prevColor = gHistoryTexture.Sample(basicSampler, pIn.uv - velocity).rgb;
	prevColor = ToneMap(prevColor);
	prevColor = RGB2YCoCgR(prevColor);

	if (gFrameCount == 0)
	{
		output.color = float4(currColor, 1.0f);
		output.history = float4(currColor, 1.0f);
		return output;
	}

	// Sample neighborhoods.

	uint N = 9;
	float TotalWeight = 0.0f;
	float3 sum = 0.0f;
	float3 m1 = 0.0f;
	float3 m2 = 0.0f;
	float3 neighborMin = float3(9999999.0f, 9999999.0f, 9999999.0f);
	float3 neighborMax = float3(-99999999.0f, -99999999.0f, -99999999.0f);
	float3 neighborhood[9];
	float neighborhoodSampWeight[9];

	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			uint i = (y + 1) * 3 + x + 1;
			float2 sampleOffset = float2(x, y) * gInvRenderTargetSize;
			float2 sampleUV = JitteredUV + sampleOffset;
			sampleUV = saturate(sampleUV);

			float3 NeighborhoodSamp = gInputTexture.Sample(basicSampler, sampleUV).rgb;
			NeighborhoodSamp = max(NeighborhoodSamp, 0.0f);
			NeighborhoodSamp = ToneMap(NeighborhoodSamp);
			NeighborhoodSamp = RGB2YCoCgR(NeighborhoodSamp);

			neighborhood[i] = NeighborhoodSamp;
			neighborhoodSampWeight[i] = HdrWeight4(NeighborhoodSamp, ExposureScale);
			neighborMin = min(neighborMin, NeighborhoodSamp);
			neighborMax = max(neighborMax, NeighborhoodSamp);

			m1 += NeighborhoodSamp;
			m2 += NeighborhoodSamp * NeighborhoodSamp;
			TotalWeight += neighborhoodSampWeight[i];
			sum += neighborhood[i] * neighborhoodSampWeight[i];
		}
	}

	// Variance clip.
	float3 mu = m1 / N;
	float3 sigma = sqrt(abs(m2 / N - mu * mu));
	float3 minc = mu - VarianceClipGamma * sigma;
	float3 maxc = mu + VarianceClipGamma * sigma;
	float3 Filtered = sum / TotalWeight;
	minc = min(minc, Filtered);
	maxc = max(maxc, Filtered);
	//prevColor = ClipAABB(colorMin, colorMax, prevColor, (colorMin + colorMax) / 2);
	prevColor = ClipAABB(minc, maxc, prevColor, mu);
	//prevColor = YCoCgR2RGB(prevColor);

#ifndef ANTI_FLICKERING
	float weightCurr = 0.1f;
	float weightPrev = 1.0f - weightCurr;
#else
	// Anti-flickering
	float weightCurrMin = 0.05f;
	float weightCurrMax = 0.12f;

	float LumaMin = Luminance(neighborMin);
	float LumaMax = Luminance(neighborMax);
	float LumaHistory = Luminance(prevColor);

	float WeightCurrBlend = 1.0f;

	float DistToClamp = 2 * abs(min(LumaHistory - LumaMin, LumaMax - LumaHistory) / (LumaMax - LumaMin));
	WeightCurrBlend *= lerp(0, 1, saturate(4 * DistToClamp));
	//BlendFinal += 0.8 * saturate(0.02 * LumaHistory / abs(Filtered.x - LumaHistory));
	//BlendFinal *= (LumaMin * InExposureScale + 0.5) / (LumaMax * InExposureScale + 0.5);

	float weightCurr = lerp(weightCurrMin, weightCurrMax, WeightCurrBlend);
	float weightPrev = 1.0f - weightCurr;
#endif

	//weightCurr *= 1.0f / (1.0f + Luminance(currColor));
	//weightPrev *= 1.0f / (1.0f + Luminance(prevColor));

	float RcpWeight = rcp(weightCurr + weightPrev);
	float3 color = (currColor * weightCurr + prevColor * weightPrev) * RcpWeight;

	color = YCoCgR2RGB(color);
	color = UnToneMap(color);

	output.color = float4(color, 1.0f);
	output.history = float4(color, 1.0f);

	//output.color = float4(float2(pIn.uv), 0.0f, 1.0f);
	return output;

}

