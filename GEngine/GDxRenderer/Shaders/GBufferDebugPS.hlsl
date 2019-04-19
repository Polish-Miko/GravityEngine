
#include "Material.hlsli"

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float2 TexC    : TEXCOORD;
};

// Constant data that varies per frame.
cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gTexTransform;
	uint gMaterialIndex;
	uint gObjPad0;
	uint gObjPad1;
	uint gObjPad2;
};

Texture2D gTextureMaps[4] : register(t0);

SamplerState Sampler	  : register(s0);

float4 main(VertexOut input) : SV_TARGET
{
	MaterialData matData = gMaterialData[gMaterialIndex];
	int index = (int)(matData.ScalarParams[0]);
	int mask = (int)(matData.ScalarParams[1]);
	float4 color;
	switch (mask)
	{
	case 0:
		color = float4(pow(gTextureMaps[index].Sample(Sampler, input.TexC).rgb, 1 / 2.2f), 1.0f);
		break;
	case 1:
		color = float4(gTextureMaps[index].Sample(Sampler, input.TexC).rrr, 1.0f);
		break;
	case 2:
		color = float4(gTextureMaps[index].Sample(Sampler, input.TexC).ggg, 1.0f);
		break;
	case 3:
		color = float4(gTextureMaps[index].Sample(Sampler, input.TexC).bbb, 1.0f);
		break;
	default:
		color = float4(gTextureMaps[index].Sample(Sampler, input.TexC).rgb, 1.0f);
		break;
	}
	return color;
}