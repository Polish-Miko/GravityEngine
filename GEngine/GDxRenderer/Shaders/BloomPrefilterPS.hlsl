
#include "StaticSamplers.hlsli"
#include "ShaderDefinition.h"

#define HALF_MAX        65504.0 // (2 - 2^-10) * 2^15	
#define EPSILON         1.0e-4

cbuffer cbBloom : register(b0)
{
	float _BloomTexelSizeX;
	float _BloomTexelSizeY;
	float _Threshold1;
	float _Threshold2;
	float _Threshold3;
	float _Threshold4;
	float _SampleScale;
};

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

// Input textures.
Texture2D gBloomChain				: register(t0);
Texture2D gBloomInput				: register(t1);

// Better, temporally stable box filtering
// [Jimenez14] http://goo.gl/eomGso
// . . . . . . .
// . A . B . C .
// . . D . E . .
// . F . G . H .
// . . I . J . .
// . K . L . M .
// . . . . . . .
float4 DownsampleBox13Tap(Texture2D tex, float2 uv, float2 texelSize)
{
	float4 A = tex.Sample(linearClampSampler, uv + texelSize * float2(-1.0, -1.0));
	float4 B = tex.Sample(linearClampSampler, uv + texelSize * float2(0.0, -1.0));
	float4 C = tex.Sample(linearClampSampler, uv + texelSize * float2(1.0, -1.0));
	float4 D = tex.Sample(linearClampSampler, uv + texelSize * float2(-0.5, -0.5));
	float4 E = tex.Sample(linearClampSampler, uv + texelSize * float2(0.5, -0.5));
	float4 F = tex.Sample(linearClampSampler, uv + texelSize * float2(-1.0, 0.0));
	float4 G = tex.Sample(linearClampSampler, uv);
	float4 H = tex.Sample(linearClampSampler, uv + texelSize * float2(1.0, 0.0));
	float4 I = tex.Sample(linearClampSampler, uv + texelSize * float2(-0.5, 0.5));
	float4 J = tex.Sample(linearClampSampler, uv + texelSize * float2(0.5, 0.5));
	float4 K = tex.Sample(linearClampSampler, uv + texelSize * float2(-1.0, 1.0));
	float4 L = tex.Sample(linearClampSampler, uv + texelSize * float2(0.0, 1.0));
	float4 M = tex.Sample(linearClampSampler, uv + texelSize * float2(1.0, 1.0));

	half2 div = (1.0 / 4.0) * half2(0.5, 0.125);

	float4 o = (D + E + I + J) * div.x;
	o += (A + B + G + F) * div.y;
	o += (B + C + H + G) * div.y;
	o += (F + G + L + K) * div.y;
	o += (G + H + M + L) * div.y;

	return o;
}

// Standard box filtering
float4 DownsampleBox4Tap(Texture2D tex, float2 uv, float2 texelSize)
{
	float4 d = texelSize.xyxy * float4(-1.0, -1.0, 1.0, 1.0);

	float4 s;
	s = (tex.Sample(linearClampSampler, uv + d.xy));
	s += (tex.Sample(linearClampSampler, uv + d.zy));
	s += (tex.Sample(linearClampSampler, uv + d.xw));
	s += (tex.Sample(linearClampSampler, uv + d.zw));

	return s * (1.0 / 4.0);
}

// Clamp HDR value within a safe range
float3 SafeHDR(float3 c)
{
	return min(c, HALF_MAX);
}

float4 SafeHDR(float4 c)
{
	return min(c, HALF_MAX);
}

//
// Quadratic color thresholding
// curve = (threshold - knee, knee * 2, 0.25 / knee)
//
float4 QuadraticThreshold(float4 color, float threshold, float3 curve)
{
	// Pixel brightness
	float br = max(max(color.r, color.g), color.b);

	// Under-threshold part: quadratic curve
	float rq = clamp(br - curve.x, 0.0, curve.y);
	rq = curve.z * rq * rq;

	// Combine and apply the brightness response curve.
	color *= max(rq, br - threshold) / max(br, EPSILON);

	return color;
}

float4 Prefilter(float4 color, float2 uv)
{
	//float autoExposure = SAMPLE_TEXTURE2D(_AutoExposureTex, sampler_AutoExposureTex, uv).r;
	//color *= autoExposure;
	color = min(BLOOM_CLAMP, color); // clamp to max
	float4 _Threshold = float4(_Threshold1, _Threshold2, _Threshold3, _Threshold4);// x: threshold value (linear), y: threshold - knee, z: knee * 2, w: 0.25 / knee
	color = QuadraticThreshold(color, _Threshold.x, _Threshold.yzw);
	return color;
}

// Velocity texture setup
float4 main(VertexToPixel i) : SV_Target
{
#if BLOOM_USE_13_TAP_FILTER
	float4 color = DownsampleBox13Tap(gBloomChain, i.uv, float2(_BloomTexelSizeX, _BloomTexelSizeY));
	return Prefilter(SafeHDR(color), i.uv);
#else
	float4 color = DownsampleBox4Tap(gBloomChain, i.uv, float2(_BloomTexelSizeX, _BloomTexelSizeY));
	return Prefilter(SafeHDR(color), i.uv);
#endif
}

