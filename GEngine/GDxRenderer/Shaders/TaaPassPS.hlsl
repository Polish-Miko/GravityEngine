
//#include "ObjectCB.hlsli"
#include "MainPassCB.hlsli"
#include "HaltonSequence.hlsli"


static const float VarianceClipGamma = 1.00f;

static const int SampleRadius = 1;


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

float Luminance(in float3 color)
{
	return dot(color, float3(0.299f, 0.587f, 0.114f));
}

PixelOutput main(VertexToPixel pIn)// : SV_TARGET
{

	PixelOutput output;

	uint subsampIndex = gFrameCount % SAMPLE_COUNT;
	float2 Jitter = float2(
		Halton_2_3[subsampIndex].x * gInvRenderTargetSize.x * JitterDistance,
		Halton_2_3[subsampIndex].y * gInvRenderTargetSize.y * JitterDistance
		) / 2;
	float2 JitteredUV = pIn.uv + Jitter;

	float2 velocity = gVelocityBuffer.Sample(basicSampler, JitteredUV).rg;

	float3 currColor = gInputTexture.Sample(basicSampler, JitteredUV).rgb;
	float3 prevColor = gHistoryTexture.Sample(basicSampler, pIn.uv - velocity).rgb;

	if (gFrameCount == 0)
	{
		output.color = float4(currColor, 1.0f);
		output.history = float4(currColor, 1.0f);
		return output;
	}

	// Sample neighborhoods.

	uint N = 0;
	float3 colorMin = float3(9999999.0f, 9999999.0f, 9999999.0f);
	float3 colorMax = float3(-99999999.0f, -99999999.0f, -99999999.0f);
	float3 m1 = 0.0f;
	float3 m2 = 0.0f;

	for (int y = -SampleRadius; y <= SampleRadius; ++y)
	{
		for (int x = -SampleRadius; x <= SampleRadius; ++x)
		{
			float2 sampleOffset = float2(x, y) * gInvRenderTargetSize;
			float2 sampleUV = JitteredUV + sampleOffset;
			sampleUV = saturate(sampleUV);

			float3 NeighborhoodSamp = gInputTexture.Sample(basicSampler, sampleUV).rgb;
			NeighborhoodSamp = max(NeighborhoodSamp, 0.0f);
			//NeighborhoodSamp = RGB2YCoCgR(NeighborhoodSamp);//remove?

			colorMin = min(colorMin, NeighborhoodSamp);
			colorMax = max(colorMax, NeighborhoodSamp);

			m1 += NeighborhoodSamp;
			m2 += NeighborhoodSamp * NeighborhoodSamp;
			N += 1;
		}
	}

	// Variance clip.
	float3 mu = m1 / N;
	float3 sigma = sqrt(abs(m2 / N - mu * mu));
	float3 minc = mu - VarianceClipGamma * sigma;
	float3 maxc = mu + VarianceClipGamma * sigma;
	//prevColor = ClipAABB(colorMin, colorMax, prevColor, (colorMin + colorMax) / 2);
	prevColor = ClipAABB(minc, maxc, prevColor, mu);
	//prevColor = YCoCgR2RGB(prevColor);

	float3 weightA = float3(0.1f, 0.1f, 0.1f);
	float3 weightB = float3(1.0f, 1.0f, 1.0f) - weightA;

	//weightA *= 1.0f / (1.0f + Luminance(currColor));
	//weightB *= 1.0f / (1.0f + Luminance(prevColor));

	//float3 color = 0.05 * light + 0.95 * prevColor;
	float3 color = (currColor * weightA + prevColor * weightB) / (weightA + weightB);

	output.color = float4(color, 1.0f);
	output.history = float4(color, 1.0f);

	//output.color = float4(float2(pIn.uv), 0.0f, 1.0f);
	return output;

}

