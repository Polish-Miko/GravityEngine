
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
	float4  cPos		: POSITION0;
	//float4  jcPos		: POSITION1;
};

VertexOutput main(VertexInput input)
{
	VertexOutput output;

	double4x4 ViewProj = double4x4(gViewProj);

	double4 worldPos = mul(double4(input.pos, 1.0f), gWorld);
	double4 outpos = mul(worldPos, gViewProj);//Jittered
	output.pos = (float4)outpos;
	output.cPos = (float4)mul(worldPos, gUnjitteredViewProj);
	//output.jcPos = (float4)mul(worldPos, gViewProj);
	return output;
}