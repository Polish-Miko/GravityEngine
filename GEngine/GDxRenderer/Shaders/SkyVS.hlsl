
#include "ShaderDefinition.h"
#include "ObjectCB.hlsli"
#include "SkyPassCB.hlsli"



TextureCube gCubeMap : register(t0);
SamplerState basicSampler	: register(s0);

struct VertexIn
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float4 PosH		:	SV_POSITION;
    float3 PosL		:	POSITION0;
	float4 curPos	:	POSITION1;
	float4 prevPos	:	POSITION2;
};
 
VertexOut main(VertexIn vin)
{
	VertexOut vout;

	// Use local vertex position as cubemap lookup vector.
	vout.PosL = vin.PosL;
	
	// Transform to world space.
	float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
	float4 prevPosW = mul(float4(vin.PosL, 1.0f), gWorld);

	// Always center sky about camera.
	posW.xyz += gEyePosW;
	prevPosW.xyz += gPrevPos;

	// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
	vout.PosH = mul(posW, gViewProj).xyww;
	vout.PosH.z = vout.PosH.w * FAR_Z_UNORM;
	vout.curPos = mul(posW, gUnjitteredViewProj);
	vout.prevPos = mul(prevPosW, gPrevViewProj);
	
	return vout;
}

