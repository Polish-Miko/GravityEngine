
#include "ObjectCB.hlsli"
#include "MainPassCB.hlsli"

struct VertexInput
{
	float3 pos		: POSITION;
	float2 uv		: TEXCOORD;
	float3 normal	: NORMAL;
	float3 tangent	: TANGENT;
};

struct VertexOutput
{
	float4	pos			: SV_POSITION;
};

VertexOutput main(VertexInput input)
{
	VertexOutput output;

	float4 worldPos = mul(float4(input.pos, 1.0f), gWorld);
	output.pos = mul(worldPos, gShadowViewProj[gFrameCount % SHADOW_CASCADE_NUM]);

	return output;
}