
#ifndef TILED_DEFERRED_CS_HLSL
#define TILED_DEFERRED_CS_HLSL


#include "ShaderDefinition.h"
#include "Lighting.hlsli"
#include "MainPassCB.hlsli"

#define USE_TBDR 1

#define TEST 0

#define VISUALIZE_TILE_LIGHT_NUM 0

//#define COMPUTE_SHADER_TILE_GROUP_SIZE (TILE_SIZE_X * TILE_SIZE_Y)

//G-Buffer
Texture2D gAlbedoTexture			: register(t0);
Texture2D gNormalTexture			: register(t1);
Texture2D gVelocityTexture			: register(t2);
Texture2D gOrmTexture				: register(t3);

Texture2D gDepthBuffer				: register(t4);

#define PREFILTER_MIP_LEVEL 5

TextureCube skyIrradianceTexture	: register(t5);
Texture2D	brdfLUTTexture			: register(t6);
TextureCube skyPrefilterTexture[PREFILTER_MIP_LEVEL]	: register(t7);

RWTexture2D<float4> gFramebuffer : register(u0);

groupshared uint sMinZ;
groupshared uint sMaxZ;

// Light list for the tile
groupshared uint sTilePointLightIndices[MAX_POINT_LIGHT_NUM];
groupshared uint sTileNumPointLights;

SamplerState samLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

float LinearDepth(float depth)
{
	return (depth * NEAR_Z) / (FAR_Z - depth * (FAR_Z - NEAR_Z));
}

float ViewDepth(float depth)
{
	return (FAR_Z * NEAR_Z) / (FAR_Z - depth * (FAR_Z - NEAR_Z));
}

float3 PrefilteredColor(float3 viewDir, float3 normal, float roughness)
{
	float roughnessLevel = roughness * PREFILTER_MIP_LEVEL;
	int fl = floor(roughnessLevel);
	int cl = ceil(roughnessLevel);
	float3 R = reflect(-viewDir, normal);
	float3 flSample = skyPrefilterTexture[fl].SampleLevel(samLinear, R, 0).rgb;
	float3 clSample = skyPrefilterTexture[cl].SampleLevel(samLinear, R, 0).rgb;
	float3 prefilterColor = lerp(flSample, clSample, (roughnessLevel - fl));
	return prefilterColor;
}

float2 BrdfLUT(float3 normal, float3 viewDir, float roughness)
{
	float NdotV = dot(normal, viewDir);
	NdotV = max(NdotV, 0.0f);
	float2 uv = float2(NdotV, roughness);
	return brdfLUTTexture.SampleLevel(samLinear, uv, 0).rg;
}

float3 ReconstructWorldPos(uint2 gCoords, float depth)
{
	float ndcX = gCoords.x * gInvRenderTargetSize.x * 2 - 1;
	float ndcY = 1 - gCoords.y * gInvRenderTargetSize.y * 2;//remember to flip y!!!
	float4 viewPos = mul(float4(ndcX, ndcY, depth, 1.0f), gInvProj);
	viewPos = viewPos / viewPos.w;
	return mul(viewPos, gInvView).xyz;
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

	float depthBuffer = gDepthBuffer.Load(int3(globalCoords, 0)).r;
	float depth = ViewDepth(depthBuffer);
	float linearDepth = (depth - NEAR_Z) / (FAR_Z - NEAR_Z);

    // Initialize shared memory light list and Z bounds
    if (groupIndex == 0)
	{
        sTileNumPointLights = 0;
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
        
        // Cull: point light sphere vs tile frustum
        bool inFrustum = true;

        [unroll]
		for (uint i = 0; i < 6; ++i)
		{
			float4 lightPositionView = mul(float4(light.Position, 1.0f), gView);
			float d = dot(frustumPlanes[i], lightPositionView);
            inFrustum = inFrustum && (d >= -light.Range);
        }

        [branch]
		if (inFrustum)
		{
            // Append light to list
            // Compaction might be better if we expect a lot of lights
            uint listIndex;
            InterlockedAdd(sTileNumPointLights, 1, listIndex);
            sTilePointLightIndices[listIndex] = lightIndex;
        }
    }
 
    GroupMemoryBarrierWithGroupSync();
    
    uint numLights = sTileNumPointLights;

    // Only process onscreen pixels (tiles can span screen edges)
	if (all(globalCoords < gRenderTargetSize.xy) && linearDepth > 0.0f)
	{
#if VISUALIZE_TILE_LIGHT_NUM
		gFramebuffer[globalCoords] = float(sTileNumPointLights) / 30.0f;
#else
		float3 finalColor = 0.f;

		float4 packedAlbedo = gAlbedoTexture.Load(int3(globalCoords, 0));
		float3 albedo = packedAlbedo.rgb;
		float3 normal = gNormalTexture.Load(int3(globalCoords, 0)).rgb;
		float roughness = gOrmTexture.Load(int3(globalCoords, 0)).g;
		float metal = gOrmTexture.Load(int3(globalCoords, 0)).b;
		float3 worldPos = ReconstructWorldPos(globalCoords, depthBuffer);

		//clamp roughness
		roughness = max(ROUGHNESS_CLAMP, roughness);

		float shadowAmount = 1.f;

		if (numLights > 0)
		{

			int i = 0;

#if USE_TBDR
			// Point light.
			for (i = 0; i < numLights; i++)
			{
				shadowAmount = 1.f;
				float atten = Attenuate(pointLight[sTilePointLightIndices[i]].Position, pointLight[sTilePointLightIndices[i]].Range, worldPos);
				float lightIntensity = pointLight[sTilePointLightIndices[i]].Intensity * atten;
				float3 toLight = normalize(pointLight[sTilePointLightIndices[i]].Position - worldPos);
				float3 lightColor = pointLight[sTilePointLightIndices[i]].Color.rgb;

				finalColor = finalColor + DirectPBR(lightIntensity, lightColor, toLight, normalize(normal), worldPos, cameraPosition, roughness, metal, albedo, shadowAmount);
			}
#else
			for (i = 0; i < MAX_POINT_LIGHT_NUM; i++)
			{
				shadowAmount = 1.f;
				float atten = Attenuate(pointLight[i].Position, pointLight[i].Range, worldPos);
				float lightIntensity = pointLight[i].Intensity * atten;
				float3 toLight = normalize(pointLight[i].Position - worldPos);
				float3 lightColor = pointLight[i].Color.rgb;

				finalColor = finalColor + DirectPBR(lightIntensity, lightColor, toLight, normalize(normal), worldPos, cameraPosition, roughness, metal, albedo, shadowAmount);
			}
#endif

			// Directional light.
			for (i = 0; i < dirLightCount; i++)
			{
				float shadowAmount = 1.f;
				float lightIntensity = dirLight[i].Intensity;
				float3 toLight = normalize(-dirLight[i].Direction);
				float3 lightColor = dirLight[i].DiffuseColor.rgb;

				finalColor = finalColor + DirectPBR(lightIntensity, lightColor, toLight, normalize(normal), worldPos, cameraPosition, roughness, metal, albedo, shadowAmount);
			}
		}
		else
		{
			finalColor = float3(0.0f, 0.0f, 0.0f);
		}

		// Ambient light.
		float3 viewDir = normalize(cameraPosition - worldPos);
		float3 prefilter = PrefilteredColor(viewDir, normal, roughness);
		float2 brdf = BrdfLUT(normal, viewDir, roughness);
		float3 irradiance = skyIrradianceTexture.SampleLevel(samLinear, normal, 0).rgb;

		finalColor = finalColor + AmbientPBR(normalize(normal), worldPos,
			cameraPosition, roughness, metal, albedo,
			irradiance, prefilter, brdf, shadowAmount);

#if TEST
		float test = depth / 50000.0f;
		finalColor = float3(linearDepth, linearDepth, linearDepth);
#endif

		gFramebuffer[globalCoords] = float4(finalColor, 1.0f);
#endif
    }


}


#endif



