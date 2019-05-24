
#ifndef CLUSTERED_DEFERRED_CS_HLSL
#define CLUSTERED_DEFERRED_CS_HLSL


#include "ShaderDefinition.h"
#include "Lighting.hlsli"
#include "MainPassCB.hlsli"

RWStructuredBuffer<LightList> gLightList : register(u0);

Texture2D gDepthBuffer				: register(t0);

[numthreads(CLUSTER_THREAD_NUM_X, CLUSTER_THREAD_NUM_Y, 1)]
void main(
	uint3 groupId          : SV_GroupID,
	uint3 dispatchThreadId : SV_DispatchThreadID,
	uint3 groupThreadId : SV_GroupThreadID
	)
{
    uint groupIndex = groupThreadId.y * CLUSTER_THREAD_NUM_X + groupThreadId.x;

    uint2 globalCoords = dispatchThreadId.xy;

	uint clusterIdStart = (groupId.y * ceil(gRenderTargetSize.x / CLUSTER_SIZE_X) + groupId.x) * CLUSTER_NUM_Z;

    // Initialize light list.
	if (groupIndex < CLUSTER_NUM_Z)
	{
		gLightList[clusterIdStart + groupIndex].NumPointLights = 0;
		gLightList[clusterIdStart + groupIndex].NumSpotlights = 0;
	}

    GroupMemoryBarrierWithGroupSync();

	// Work out scale/bias from [0, 1]
	float2 tileNum = float2(gRenderTargetSize.xy) * rcp(float2(CLUSTER_SIZE_X, CLUSTER_SIZE_Y));
	float2 tileCenterOffset = float2(groupId.xy) * 2 + float2(1.0f, 1.0f) - tileNum;

	// Now work out composite projection matrix
	// Relevant matrix columns for this tile frusta
	float4 c1 = float4(-gProj._11 * tileNum.x, 0.0f, tileCenterOffset.x, 0.0f);
	float4 c2 = float4(0.0f, -gProj._22 * tileNum.y, -tileCenterOffset.y, 0.0f);
	float4 c4 = float4(0.0f, 0.0f, 1.0f, 0.0f);

    // Derive frustum planes
    float4 frustumPlanes[6];
    // Sides
    frustumPlanes[0] = c4 - c1;
    frustumPlanes[1] = c4 + c1;
    frustumPlanes[2] = c4 - c2;
    frustumPlanes[3] = c4 + c2;

    // Normalize frustum planes (near/far already normalized)
    [unroll]
	for (uint i = 0; i < 4; ++i)
	{
        frustumPlanes[i] *= rcp(length(frustumPlanes[i].xyz));  
    }

    // Cull lights for this cluster
	for (uint lightIndex = groupIndex; lightIndex < (uint)pointLightCount; lightIndex += COMPUTE_SHADER_CLUSTER_GROUP_SIZE)
	{
		bool inFrustum = true;

		float4 lightPositionView = mul(float4(pointLight[lightIndex].Position, 1.0f), gView);

		[unroll]
		for (uint i = 0; i < 4; ++i)
		{
			float d = dot(frustumPlanes[i], lightPositionView);
			inFrustum = inFrustum && (d >= -pointLight[lightIndex].Range);
		}

		if (inFrustum)
		{
			[unroll]
			for (uint clusterZ = 0; clusterZ < CLUSTER_NUM_Z; clusterZ++)
			{
				// Cull: point light sphere vs cluster frustum
				inFrustum = true;

				// Near/far
				frustumPlanes[4] = float4(0.0f, 0.0f, 1.0f, -DepthSlicing_16[clusterZ]);
				frustumPlanes[5] = float4(0.0f, 0.0f, -1.0f, DepthSlicing_16[clusterZ + 1]);

				[unroll]
				for (uint i = 4; i < 6; ++i)
				{
					float d = dot(frustumPlanes[i], lightPositionView);
					inFrustum = inFrustum && (d >= -pointLight[lightIndex].Range);
				}

				[branch]
				if (inFrustum)
				{
					// Append light to list
					// Compaction might be better if we expect a lot of lights
					uint listIndex = 0;
					InterlockedAdd(gLightList[clusterIdStart + clusterZ].NumPointLights, 1, listIndex);
					if (listIndex < MAX_GRID_POINT_LIGHT_NUM)
					{
						gLightList[clusterIdStart + clusterZ].PointLightIndices[listIndex] = lightIndex;
					}
				}
			}
		}

    }
}


#endif



