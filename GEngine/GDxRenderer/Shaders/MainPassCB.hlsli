
#ifndef _MAINPASSCB_HLSLI
#define _MAINPASSCB_HLSLI



// Main pass cb.
cbuffer cbPass : register(b1)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gUnjitteredViewProj;
	float4x4 gInvViewProj;
	float4x4 gPrevViewProj;
	float4x4 gViewProjTex;
	float4x4 gShadowTransform;
	float3 gEyePosW;
	float cbPerObjectPad1;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
	uint gFrameCount;
	float2 gJitter;
	uint gPad0;
	float4 gAmbientLight;
	float4 gMainDirectionalLightDir;
};

#endif 