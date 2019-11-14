
#ifndef SDF_TILE_CS_HLSL
#define SDF_TILE_CS_HLSL


#include "ShaderDefinition.h"
#include "SdfDescriptors.hlsli"
#include "MainPassCB.hlsli"

struct SdfList
{
	uint NumSdf;
	uint SdfObjIndices[MAX_GRID_SDF_NUM];
};

RWStructuredBuffer<SdfList> gSdfList : register(u0);

SamplerState samLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

[numthreads(SDF_TILE_THREAD_NUM_X, SDF_TILE_THREAD_NUM_Y, 1)]
void main(
	uint3 groupId          : SV_GroupID,	
	uint3 dispatchThreadId : SV_DispatchThreadID,
	uint3 groupThreadId : SV_GroupThreadID
	)
{
	//uint tileIndex = dispatchThreadId.y * SDF_GRID_NUM + dispatchThreadId.x;
	//gSdfList[tileIndex].NumSdf = 0;

	uint tileIndex = dispatchThreadId.y * SDF_GRID_NUM + dispatchThreadId.x;

	float tileWidth = gSdfTileSpaceSize.x / SDF_GRID_NUM;
	float tileXmin = -gSdfTileSpaceSize.x / 2.0f + dispatchThreadId.x * tileWidth;
	float tileXmax = -gSdfTileSpaceSize.x / 2.0f + (dispatchThreadId.x + 1) * tileWidth;
	float tileHeight = gSdfTileSpaceSize.y / SDF_GRID_NUM;
	float tileYmin = -gSdfTileSpaceSize.y / 2.0f + dispatchThreadId.y * tileHeight;
	float tileYmax = -gSdfTileSpaceSize.y / 2.0f + (dispatchThreadId.y + 1) * tileHeight;

	gSdfList[tileIndex].NumSdf = 0;

	float xmin = 0.0f;
	float ymin = 0.0f;
	float xmax = 0.0f;
	float ymax = 0.0f;
	float2 center = float2(0.0f, 0.0f);
	float radius = 0.0f;

	for (int i = 0; i < gSceneObjectNum; i++)
	{
		center = gSceneObjectSdfDescriptors[i].objLightSpaceCenter.xy;
		radius = gSceneObjectSdfDescriptors[i].objWorldRadius;

		xmin = tileXmin - radius;
		xmax = tileXmax + radius;
		ymin = tileYmin - radius;
		ymax = tileYmax + radius;

		if (center.x > xmin &&
			center.x < xmax &&
			center.y > ymin &&
			center.y < ymax)
		{
			gSdfList[tileIndex].SdfObjIndices[gSdfList[tileIndex].NumSdf] = i;
			gSdfList[tileIndex].NumSdf++;
		}
	}

}


#endif



