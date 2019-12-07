
#include "StaticSamplers.hlsli"
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"

#define USE_MOTION_BLUR 0

#define TWO_PI          6.28318530718

cbuffer cbMotionBlur : register(b0)
{
	float NeighborMaxTexelSizeX;
	float NeighborMaxTexelSizeY;
	int LoopCount;
};

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

// Input textures.
Texture2D gInputTexture					: register(t0);
Texture2D gVelocityDepthTexture			: register(t1);
Texture2D gNeighborMaxTexture			: register(t2);

//********************************************************************************************************************************************************************
// Util.
//********************************************************************************************************************************************************************

// Interleaved gradient function from Jimenez 2014
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
float GradientNoise(float2 uv)
{
	uv = floor(uv * gRenderTargetSize.xy);
	float f = dot(float2(0.06711056, 0.00583715), uv);
	return frac(52.9829189 * frac(f));
}

// Returns true or false with a given interval.
bool Interval(float phase, float interval)
{
	return frac(phase / interval) > 0.499;
}

// Jitter function for tile lookup
float2 JitterTile(float2 uv)
{
	float rx, ry;
	sincos(GradientNoise(uv + float2(2.0, 0.0)) * TWO_PI, ry, rx);
	return float2(rx, ry) * float2(NeighborMaxTexelSizeX, NeighborMaxTexelSizeY) * 0.25;
}

// Velocity sampling function
float3 SampleVelocity(float2 uv)
{
	float3 v = gVelocityDepthTexture.SampleLevel(linearClampSampler, uv, 0.0f).xyz;
	return float3((v.xy * 2.0 - 1.0) * MOTION_BLUR_MAX_RADIUS, v.z);
}

//********************************************************************************************************************************************************************
// Reconstruction.
//********************************************************************************************************************************************************************

float4 main(VertexToPixel i) : SV_TARGET
{

#if USE_MOTION_BLUR

	// Color sample at the center point
	const float4 c_p = gInputTexture.Sample(linearClampSampler, i.uv);
	
	// Velocity/Depth sample at the center point
	const float3 vd_p = SampleVelocity(i.uv);
	const float l_v_p = max(length(vd_p.xy), 0.5);
	const float rcp_d_p = 1.0 / vd_p.z;
	
	// NeighborMax vector sample at the center point
	const float2 v_max = gNeighborMaxTexture.Sample(linearClampSampler, i.uv + JitterTile(i.uv)).xy;
	const float l_v_max = length(v_max);
	const float rcp_l_v_max = 1.0 / l_v_max;

	// Escape early if the NeighborMax vector is small enough.
	if (l_v_max < 2.0) return c_p;
	
	// Use V_p as a secondary sampling direction except when it's too small
	// compared to V_max. This vector is rescaled to be the length of V_max.
	const float2 v_alt = (l_v_p * 2.0 > l_v_max) ? vd_p.xy * (l_v_max / l_v_p) : v_max;
	
	// Determine the sample count.
	const float sc = floor(min(LoopCount, l_v_max * 0.5));
	
	// Loop variables (starts from the outermost sample)
	const float dt = 1.0 / sc;
	const float t_offs = (GradientNoise(i.uv) - 0.5) * dt;
	float t = 1.0 - dt * 0.5;
	float count = 0.0;
	
	// Background velocity
	// This is used for tracking the maximum velocity in the background layer.
	float l_v_bg = max(l_v_p, 1.0);
	
	// Color accumlation
	float4 acc = 0.0;
	
	[loop]
	while (t > dt * 0.25)
	{
		// Sampling direction (switched per every two samples)
		const float2 v_s = Interval(count, 4.0) ? v_alt : v_max;

		// Sample position (inverted per every sample)
		const float t_s = (Interval(count, 2.0) ? -t : t) + t_offs;

		// Distance to the sample position
		const float l_t = l_v_max * abs(t_s);

		// UVs for the sample position
		const float2 uv0 = i.uv + v_s * t_s * gInvRenderTargetSize.xy;
		const float2 uv1 = i.uv + v_s * t_s * gInvRenderTargetSize.xy;

		// Color sample
		const float3 c = gInputTexture.SampleLevel(linearClampSampler, uv0, 0.0f).rgb;
		// Velocity/Depth sample
		const float3 vd = SampleVelocity(uv1);

		// Background/Foreground separation
		const float fg = saturate((vd_p.z - vd.z) * 20.0 * rcp_d_p);

		// Length of the velocity vector
		const float l_v = lerp(l_v_bg, length(vd.xy), fg);

		// Sample weight
		// (Distance test) * (Spreading out by motion) * (Triangular window)
		const float w = saturate(l_v - l_t) / l_v * (1.2 - t);

		// Color accumulation
		acc += float4(c, 1.0) * w;

		// Update the background velocity.
		l_v_bg = max(l_v_bg, l_v);

		// Advance to the next sample.
		t = Interval(count, 2.0) ? t - dt : t;
		count += 1.0;
	}

	// Add the center sample.
	acc += float4(c_p.rgb, 1.0) * (1.2 / (l_v_bg * sc * 2.0));

	return float4(acc.rgb / acc.a, c_p.a);

#else

	float3 color = gInputTexture.Sample(linearClampSampler, i.uv).rgb;
	return float4(color, 1.0f);

#endif

}

