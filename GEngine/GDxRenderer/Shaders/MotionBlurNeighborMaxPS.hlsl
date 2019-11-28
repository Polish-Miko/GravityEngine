
#include "StaticSamplers.hlsli"
#include "ShaderUtil.hlsli"
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"

cbuffer cbMotionBlur : register(b0)
{
	float gTexelSizeX;
	float gTexelSizeY;
	float gTileMaxOffsetX;
	float gTileMaxOffsetY;
	int gTileMaxLoop;
};

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

// Input textures.
Texture2D gInputTexture				: register(t0);

float2 MaxV(float2 v1, float2 v2)
{
	return dot(v1, v1) < dot(v2, v2) ? v2 : v1;
}

// Velocity texture setup
float2 main(VertexToPixel i) : SV_Target
{
	const float cw = 1.01; // Center weight tweak

	float4 d = float4(gTexelSizeX, gTexelSizeY, gTexelSizeX, gTexelSizeY) * float4(1.0, 1.0, -1.0, 0.0);

	float2 v1 = gInputTexture.Sample(basicSampler, i.uv - d.xy).rg;
	float2 v2 = gInputTexture.Sample(basicSampler, i.uv - d.wy).rg;
	float2 v3 = gInputTexture.Sample(basicSampler, i.uv - d.zy).rg;

	float2 v4 = gInputTexture.Sample(basicSampler, i.uv - d.xw).rg;
	float2 v5 = gInputTexture.Sample(basicSampler, i.uv).rg * cw;
	float2 v6 = gInputTexture.Sample(basicSampler, i.uv + d.xw).rg;

	float2 v7 = gInputTexture.Sample(basicSampler, i.uv + d.zy).rg;
	float2 v8 = gInputTexture.Sample(basicSampler, i.uv + d.wy).rg;
	float2 v9 = gInputTexture.Sample(basicSampler, i.uv + d.xy).rg;

	float2 va = MaxV(v1, MaxV(v2, v3));
	float2 vb = MaxV(v4, MaxV(v5, v6));
	float2 vc = MaxV(v7, MaxV(v8, v9));

	return MaxV(va, MaxV(vb, vc)) * (1.0 / cw);
}

