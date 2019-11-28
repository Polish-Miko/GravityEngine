
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
	float2 InputTexelSize = float2(gTexelSizeX, gTexelSizeY);
	float2 TileMaxOffset = float2(gTileMaxOffsetX, gTileMaxOffsetY);
	float2 uv0 = i.uv + InputTexelSize.xy * TileMaxOffset.xy;

	float2 du = float2(gTexelSizeX, 0.0);
	float2 dv = float2(0.0, gTexelSizeY);

	float2 vo = 0.0;

	[loop]
	for (int ix = 0; ix < gTileMaxLoop; ix++)
	{
		[loop]
		for (int iy = 0; iy < gTileMaxLoop; iy++)
		{
			float2 uv = uv0 + du * ix + dv * iy;
			vo = MaxV(vo, gInputTexture.Sample(basicSampler, uv).rg);
		}
	}

	return vo;
}

