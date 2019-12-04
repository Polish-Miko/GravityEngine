
#include "StaticSamplers.hlsli"
#include "ShaderDefinition.h"

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

// Velocity texture setup
float4 main(VertexToPixel i) : SV_Target
{
#if BLOOM_USE_13_TAP_FILTER
	float4 color = DownsampleBox13Tap(gBloomChain, i.uv, float2(_BloomTexelSizeX, _BloomTexelSizeY));
	return color;
#else
	float4 color = DownsampleBox4Tap(gBloomChain, i.uv, float2(_BloomTexelSizeX, _BloomTexelSizeY));
	return color;
#endif
}

