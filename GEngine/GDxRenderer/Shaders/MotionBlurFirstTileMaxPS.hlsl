
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
	float4 d = float4(gTexelSizeX, gTexelSizeY, gTexelSizeX, gTexelSizeY) * float4(-0.5, -0.5, 0.5, 0.5);

	float2 v1 = gInputTexture.Sample(basicSampler, i.uv + d.xy).rg;
	float2 v2 = gInputTexture.Sample(basicSampler, i.uv + d.zy).rg;
	float2 v3 = gInputTexture.Sample(basicSampler, i.uv + d.xw).rg;
	float2 v4 = gInputTexture.Sample(basicSampler, i.uv + d.zw).rg;

	v1 = (v1 * 2.0 - 1.0) * MOTION_BLUR_MAX_RADIUS;
	v2 = (v2 * 2.0 - 1.0) * MOTION_BLUR_MAX_RADIUS;
	v3 = (v3 * 2.0 - 1.0) * MOTION_BLUR_MAX_RADIUS;
	v4 = (v4 * 2.0 - 1.0) * MOTION_BLUR_MAX_RADIUS;

	return float2(MaxV(MaxV(MaxV(v1, v2), v3), v4));
}

