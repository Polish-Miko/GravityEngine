// Copyright 2010 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.

#ifndef TILED_DEFERRED_CS_HLSL
#define TILED_DEFERRED_CS_HLSL

#define USE_TBDR 1

#include "FrustumZ.h"
#include "Lighting.hlsli"
#include "MainPassCB.hlsli"

//#define VISUALIZE_TILE_LIGHT_NUM 1

//#include "GBuffer.hlsl"
//#include "FramebufferFlat.hlsl"
//#include "ShaderDefines.h"

//G-Buffer
Texture2D gAlbedoTexture			: register(t0);
Texture2D gNormalTexture			: register(t1);
//Texture2D gWorldPosTexture		: register(t2);
Texture2D gVelocityTexture			: register(t2);
Texture2D gOrmTexture				: register(t3);

Texture2D gDepthBuffer				: register(t4);
//Texture2D<uint2> gStencilBuffer	: register(t6);

#define PREFILTER_MIP_LEVEL 5

TextureCube skyIrradianceTexture	: register(t5);
Texture2D	brdfLUTTexture			: register(t6);
TextureCube skyPrefilterTexture[PREFILTER_MIP_LEVEL]	: register(t7);

// This determines the tile size for light binning and associated tradeoffs
// should be the same with GDxRenderer.h
#define TILE_SIZE_X 16
#define TILE_SIZE_Y 16

#define COMPUTE_SHADER_TILE_GROUP_SIZE (TILE_SIZE_X * TILE_SIZE_Y)

//RWStructuredBuffer<uint2> gFramebuffer : register(u0);
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
	//float3 flSample = skyPrefilterTexture[fl].Sample(basicSampler, R).rgb;
	//float3 clSample = skyPrefilterTexture[cl].Sample(basicSampler, R).rgb;
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
	//return brdfLUTTexture.Sample(basicSampler, uv).rg;
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

// List of pixels that require per-sample shading
// We encode two 16-bit x/y coordinates in one uint to save shared memory space
//groupshared uint sPerSamplePixels[COMPUTE_SHADER_TILE_GROUP_SIZE];
//groupshared uint sNumPerSamplePixels;

//--------------------------------------------------------------------------------------
// Utility for writing to our flat MSAAed UAV
/*
void WriteSample(uint2 coords, uint sampleIndex, float4 value)
{
    gFramebuffer[GetFramebufferSampleAddress(coords, sampleIndex)] = PackRGBA16(value);
}

// Pack and unpack two <=16-bit coordinates into a single uint
uint PackCoords(uint2 coords)
{
    return coords.y << 16 | coords.x;
}
uint2 UnpackCoords(uint coords)
{
    return uint2(coords & 0xFFFF, coords >> 16);
}
*/

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

    //SurfaceData surfaceSamples[MSAA_SAMPLES];
    //ComputeSurfaceDataFromGBufferAllSamples(globalCoords, surfaceSamples);
        
    // Work out Z bounds for our samples
	/*
    float minZSample = gFarZ;
    float maxZSample = gNearZ;
    {
        [unroll] for (uint sample = 0; sample < MSAA_SAMPLES; ++sample) {
            // Avoid shading skybox/background or otherwise invalid pixels
            float viewSpaceZ = surfaceSamples[sample].positionView.z;
            bool validPixel = 
                 viewSpaceZ >= mCameraNearFar.x &&
                 viewSpaceZ <  mCameraNearFar.y;
            [flatten] if (validPixel) {
                minZSample = min(minZSample, viewSpaceZ);
                maxZSample = max(maxZSample, viewSpaceZ);
            }
        }
    }
	*/
	float depthBuffer = gDepthBuffer.Load(int3(globalCoords, 0)).r;
	float depth = ViewDepth(depthBuffer);
	float linearDepth = (depth - NEAR_Z) / (FAR_Z - NEAR_Z);
	//float stencil = gStencilBuffer.Load(int3(globalCoords, 0)).g;
    
    // Initialize shared memory light list and Z bounds
    if (groupIndex == 0)
	{
        sTileNumPointLights = 0;
        //sNumPerSamplePixels = 0;
        sMinZ = 0x7F7FFFFF;      // Max float
        sMaxZ = 0;
    }

    GroupMemoryBarrierWithGroupSync();
    
    // NOTE: Can do a parallel reduction here but now that we have MSAA and store sample frequency pixels
    // in shaded memory the increased shared memory pressure actually *reduces* the overall speed of the kernel.
    // Since even in the best case the speed benefit of the parallel reduction is modest on current architectures
    // with typical tile sizes, we have reverted to simple atomics for now.
    // Only scatter pixels with actual valid samples in them
	/*
    if (maxZSample >= minZSample) {
        InterlockedMin(sMinZ, asuint(minZSample));
        InterlockedMax(sMaxZ, asuint(maxZSample));
    }
	*/

	InterlockedMin(sMinZ, asuint(depth));
	InterlockedMax(sMaxZ, asuint(depth));

    GroupMemoryBarrierWithGroupSync();

    float minTileZ = asfloat(sMinZ);
    float maxTileZ = asfloat(sMaxZ);
    
    // NOTE: This is all uniform per-tile (i.e. no need to do it per-thread) but fairly inexpensive
    // We could just precompute the frusta planes for each tile and dump them into a constant buffer...
    // They don't change unless the projection matrix changes since we're doing it in view space.
    // Then we only need to compute the near/far ones here tightened to our actual geometry.
    // The overhead of group synchronization/LDS or global memory lookup is probably as much as this
    // little bit of math anyways, but worth testing.

	/*
	// Work out scale/bias from [0, 1]
	float2 tileScale = float2(mFramebufferDimensions.xy) * rcp(float(2 * float2(TILE_SIZE_X, TILE_SIZE_Y)));
	float2 tileBias = tileScale - float2(groupId.xy);

	// Now work out composite projection matrix
	// Relevant matrix columns for this tile frusta
	float4 c1 = float4(mCameraProj._11 * tileScale.x, 0.0f, tileBias.x, 0.0f);
	float4 c2 = float4(0.0f, -mCameraProj._22 * tileScale.y, tileBias.y, 0.0f);
	float4 c4 = float4(0.0f, 0.0f, 1.0f, 0.0f);

	float4 frustumPlanes[6];
	// Sides
	frustumPlanes[0] = c4 - c1;
	frustumPlanes[1] = c4 + c1;
	frustumPlanes[2] = c4 - c2;
	frustumPlanes[3] = c4 + c2;
	*/

	/*
	float2 tileScale = float2(gRenderTargetSize.xy) * rcp(2 * float2(TILE_SIZE_X, TILE_SIZE_Y));
	float2 tileBias = tileScale - groupId.xy;

	float4 c1 = float4(gProj._11 * tileScale.x, 0.0, tileBias.x, 0.0);
	float4 c2 = float4(0.0, -gProj._22 * tileScale.y, tileBias.y, 0.0);
	float4 c4 = float4(0.0, 0.0, 1.0, 0.0);

	float4 frustumPlanes[6];
	// Sides
	frustumPlanes[0] = c4 - c1; // right
	frustumPlanes[1] = c1; // left
	frustumPlanes[2] = c4 - c2; // top
	frustumPlanes[3] = c2; // bottom
	*/

	///*
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
	//*/

    // Near/far
    frustumPlanes[4] = float4(0.0f, 0.0f,  1.0f, -minTileZ);
    frustumPlanes[5] = float4(0.0f, 0.0f, -1.0f,  maxTileZ);
    
    // Normalize frustum planes (near/far already normalized)
    [unroll]
	for (uint i = 0; i < 6; ++i)
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
		for (uint i = 0; i < 4; ++i)
		{
			float4 lightPositionView = mul(float4(light.Position, 1.0f), gView);
            //float d = dot(frustumPlanes[i], float4(light.positionView, 1.0f));
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
	if (all(globalCoords < gRenderTargetSize.xy) && linearDepth < 1.0f)
	{
#if VISUALIZE_TILE_LIGHT_NUM
		gFramebuffer[globalCoords] = float(sTileNumPointLights) / 30.0f;
#else
		float3 finalColor = 0.f;

		float4 packedAlbedo = gAlbedoTexture.Load(int3(globalCoords, 0));
		float3 albedo = packedAlbedo.rgb;
		float3 normal = gNormalTexture.Load(int3(globalCoords, 0)).rgb;
		//float3 worldPos = gWorldPosTexture.Load(int3(globalCoords, 0)).rgb;
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
			finalColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
		}

		// Ambient light.
		float3 viewDir = normalize(cameraPosition - worldPos);
		float3 prefilter = PrefilteredColor(viewDir, normal, roughness);
		float2 brdf = BrdfLUT(normal, viewDir, roughness);
		//float3 irradiance = skyIrradianceTexture.Sample(basicSampler, normal).rgb;
		float3 irradiance = skyIrradianceTexture.SampleLevel(samLinear, normal, 0).rgb;

		finalColor = finalColor + AmbientPBR(normalize(normal), worldPos,
			cameraPosition, roughness, metal, albedo,
			irradiance, prefilter, brdf, shadowAmount);

		gFramebuffer[globalCoords] = float4(finalColor, 1.0f);
#endif
    }

	/*
    #if DEFER_PER_SAMPLE && MSAA_SAMPLES > 1
        // NOTE: We were careful to write only sample 0 above if we are going to do sample
        // frequency shading below, so we don't need a device memory barrier here.
        GroupMemoryBarrierWithGroupSync();

        // Now handle any pixels that require per-sample shading
        // NOTE: Each pixel requires MSAA_SAMPLES - 1 additional shading passes
        const uint shadingPassesPerPixel = MSAA_SAMPLES - 1;
        uint globalSamples = sNumPerSamplePixels * shadingPassesPerPixel;

        for (uint globalSample = groupIndex; globalSample < globalSamples; globalSample += COMPUTE_SHADER_TILE_GROUP_SIZE) {
            uint listIndex = globalSample / shadingPassesPerPixel;
            uint sampleIndex = globalSample % shadingPassesPerPixel + 1;        // sample 0 has been handled earlier

            uint2 sampleCoords = UnpackCoords(sPerSamplePixels[listIndex]);
            SurfaceData surface = ComputeSurfaceDataFromGBufferSample(sampleCoords, sampleIndex);

            float3 lit = float3(0.0f, 0.0f, 0.0f);
            for (uint tileLightIndex = 0; tileLightIndex < numLights; ++tileLightIndex) {
                PointLight light = gLight[sTilePointLightIndices[tileLightIndex]];
                AccumulateBRDF(surface, light, lit);
            }
            WriteSample(sampleCoords, sampleIndex, float4(lit, 1.0f));
        }
    #endif
		*/
}


#endif



