
#ifndef _OBJECTCB_HLSLI
#define _OBJECTCB_HLSLI



// Constant data that varies per frame.

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gPrevWorld;
	float4x4 gInvTransWorld;
	float4x4 gTexTransform;
	uint gMaterialIndex;
	uint gObjPad0;
	uint gObjPad1;
	uint gObjPad2;
};

#endif 