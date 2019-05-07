
#ifndef _SKYPASSCB_HLSLI
#define _SKYPASSCB_HLSLI




// Sky pass cb.
cbuffer cbPass : register(b1)
{
	float4x4 gViewProj;
	float4x4 gPrevViewProj;
	float4x4 gUnjitteredViewProj;
	float3	 gPrevPos;
	float	 pad1;
	float3 gEyePosW;
	float roughnessCb;
};

#endif 