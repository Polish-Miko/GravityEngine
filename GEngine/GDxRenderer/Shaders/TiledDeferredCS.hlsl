
#ifndef TILED_DEFERRED_CS_HLSL
#define TILED_DEFERRED_CS_HLSL


#include "ShaderDefinition.h"
#include "Lighting.hlsli"
#include "MainPassCB.hlsli"

Texture2D gDepthBuffer				: register(t0);

groupshared uint sMinZ;
groupshared uint sMaxZ;

RWStructuredBuffer<LightList> gLightList : register(u0);

SamplerState samLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

float ViewDepth(float depth)
{
	return (FAR_Z * NEAR_Z) / (FAR_Z - depth * (FAR_Z - NEAR_Z));
}

float LinearDepth(float depth)
{
	return (depth * NEAR_Z) / (FAR_Z - depth * (FAR_Z - NEAR_Z));
}

[numthreads(TILE_SIZE_X, TILE_SIZE_Y, 1)]
void main(
	uint3 groupId          : SV_GroupID,
	uint3 dispatchThreadId : SV_DispatchThreadID,
	uint3 groupThreadId : SV_GroupThreadID
	)
{
    // NOTE: This is currently necessary rather than just using SV_GroupIndex to work
    // around a compiler bug on Fermi.
    uint groupIndex = groupThreadId.y * TILE_SIZE_X + groupThreadId.x;

    uint2 globalCoords = dispatchThreadId.xy;

	uint tileId = (groupId.y * ceil(gRenderTargetSize.x / TILE_SIZE_X) + groupId.x);

	float depthBuffer = gDepthBuffer.Load(int3(globalCoords, 0)).r;
	float depth = ViewDepth(depthBuffer);
	//float linearDepth = (depth - NEAR_Z) / (FAR_Z - NEAR_Z);

    // Initialize shared memory light list and Z bounds
    if (groupIndex == 0)
	{
		gLightList[tileId].NumPointLights = 0;
		gLightList[tileId].NumSpotlights = 0;
        sMinZ = 0x7F7FFFFF;      // Max float
        sMaxZ = 0;
    }

    GroupMemoryBarrierWithGroupSync();

	InterlockedMin(sMinZ, asuint(depth));
	InterlockedMax(sMaxZ, asuint(depth));

    GroupMemoryBarrierWithGroupSync();

    float minTileZ = asfloat(sMinZ);
    float maxTileZ = asfloat(sMaxZ);

	// Work out scale/bias from [0, 1]
	float2 tileNum = float2(gRenderTargetSize.xy) * rcp(float2(TILE_SIZE_X, TILE_SIZE_Y));
	float2 tileCenterOffset = float2(groupId.xy) * 2 + float2(1.0f, 1.0f) - tileNum;

	// Now work out composite projection matrix
	// Relevant matrix columns for this tile frusta
	float4 c1 = float4(-gProj._11 * tileNum.x, 0.0f, tileCenterOffset.x, 0.0f);
	float4 c2 = float4(0.0f, -gProj._22 * tileNum.y, -tileCenterOffset.y, 0.0f);
	float4 c4 = float4(0.0f, 0.0f, 1.0f, 0.0f);

    // Derive frustum planes
	// See http://www.lighthouse3d.com/tutorials/view-frustum-culling/clip-space-approach-extracting-the-planes/
	// 
    float4 frustumPlanes[6];
    // Sides
    frustumPlanes[0] = c4 - c1;
    frustumPlanes[1] = c4 + c1;
    frustumPlanes[2] = c4 - c2;
    frustumPlanes[3] = c4 + c2;

    // Near/far
    frustumPlanes[4] = float4(0.0f, 0.0f,  1.0f, -minTileZ);
    frustumPlanes[5] = float4(0.0f, 0.0f, -1.0f,  maxTileZ);
    
    // Normalize frustum planes (near/far already normalized)
    [unroll]
	for (uint i = 0; i < 4; ++i)
	{
        frustumPlanes[i] *= rcp(length(frustumPlanes[i].xyz));
    }
    
    // Cull lights for this tile
    for (uint lightIndex = groupIndex; lightIndex < pointLightCount; lightIndex += COMPUTE_SHADER_TILE_GROUP_SIZE)
	{
        PointLight light = pointLight[lightIndex];

		float4 lightPositionView = mul(float4(pointLight[lightIndex].Position, 1.0f), gView);
        
        // Cull: point light sphere vs tile frustum
        bool inFrustum = true;

        [unroll]
		for (uint i = 0; i < 6; ++i)
		{
			//float4 lightPositionView = mul(float4(light.Position, 1.0f), gView);
			float d = dot(frustumPlanes[i], lightPositionView);
            inFrustum = inFrustum && (d >= -light.Range);
        }

        [branch]
		if (inFrustum)
		{
            // Append light to list
            // Compaction might be better if we expect a lot of lights
            uint listIndex;
            InterlockedAdd(gLightList[tileId].NumPointLights, 1, listIndex);
			if (listIndex < MAX_GRID_POINT_LIGHT_NUM)
			{
				gLightList[tileId].PointLightIndices[listIndex] = lightIndex;
			}
        }
    }


}


#endif



