
#ifndef _SDF_DESCRIPTORS_HLSLI
#define _SDF_DESCRIPTORS_HLSLI

#include "ShaderDefinition.h"





cbuffer cbSDF : register(b0)
{
	uint gSceneObjectNum;
	//uint gPad0;
	//uint gPad1;
	//uint gPad2;
};

struct MeshSdfDescriptor
{
	float3 Center;
	float HalfExtent;
	float Radius;
	int Resolution;
};

struct SceneObjectSdfDescriptor
{
	float4 objWorldSpaceCenter;
	float4 objLightSpaceCenter;
	float4x4 objWorld;
	float4x4 objInvWorld;
	float4x4 objInvWorld_IT;
	int SdfIndex;
	float objWorldRadius;
};

StructuredBuffer<MeshSdfDescriptor> gMeshSdfDescriptors : register(t0);
StructuredBuffer<SceneObjectSdfDescriptor> gSceneObjectSdfDescriptors : register(t1);

#endif 