
#ifndef CLUSTERED_DEFERRED_CS_HLSL
#define CLUSTERED_DEFERRED_CS_HLSL

#include "FrustumZ.h"
#include "Lighting.hlsli"
#include "MainPassCB.hlsli"

#define USE_CBDR 1

#define TEST 0

#define VISUALIZE_CLUSTER_LIGHT_NUM 0

#define VISUALIZE_CLUSTER_DISTRIBUTION 0

// This determines the cluster size for light binning and associated tradeoffs
// should be the same with GDxRenderer.h
#define CLUSTER_SIZE_X 32
#define CLUSTER_SIZE_Y 32
#define CLUSTER_NUM_Z 16

#define COMPUTE_SHADER_CLUSTER_GROUP_SIZE (CLUSTER_SIZE_X * CLUSTER_SIZE_Y)

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

// Light list for the tile
groupshared uint sClusterPointLightIndices[MAX_POINT_LIGHT_NUM];
groupshared uint sClusterNumPointLights;

static const float DepthSlicing_16[17] = {
	1.0f, 20.0f, 29.7f, 44.0f, 65.3f,
	96.9f, 143.7f, 213.2f, 316.2f, 469.1f,
	695.9f, 1032.4f, 1531.5f, 2272.0f, 3370.5f,
	5000.0f, 50000.0f
};

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

[numthreads(CLUSTER_SIZE_X, CLUSTER_SIZE_Y, 1)]
void main(
	uint3 groupId          : SV_GroupID,
	uint3 dispatchThreadId : SV_DispatchThreadID,
	uint3 groupThreadId : SV_GroupThreadID
	)
{
	uint test1 = 0;

    // NOTE: This is currently necessary rather than just using SV_GroupIndex to work
    // around a compiler bug on Fermi.
    uint groupIndex = groupThreadId.y * CLUSTER_SIZE_X + groupThreadId.x;

    uint2 globalCoords = dispatchThreadId.xy;

	float depthBuffer = gDepthBuffer.Load(int3(globalCoords, 0)).r;
	float depth = ViewDepth(depthBuffer);
	float linearDepth = (depth - NEAR_Z) / (FAR_Z - NEAR_Z);

    // Initialize shared memory light list and Z bounds
    if (groupIndex == 0)
	{
        sClusterNumPointLights = 0;
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
	// See http://www.lighthouse3d.com/tutorials/view-frustum-culling/clip-space-approach-extracting-the-planes/
	// 
    float4 frustumPlanes[6];
    // Sides
    frustumPlanes[0] = c4 - c1;
    frustumPlanes[1] = c4 + c1;
    frustumPlanes[2] = c4 - c2;
    frustumPlanes[3] = c4 + c2;

	// Near/far
	frustumPlanes[4] = float4(0.0f, 0.0f, 1.0f, -DepthSlicing_16[groupId.z]);
	frustumPlanes[5] = float4(0.0f, 0.0f, -1.0f, DepthSlicing_16[groupId.z + 1]);

    // Normalize frustum planes (near/far already normalized)
    [unroll]
	for (uint i = 0; i < 4; ++i)
	{
        frustumPlanes[i] *= rcp(length(frustumPlanes[i].xyz));
    }

    // Cull lights for this tile
    for (uint lightIndex = groupIndex; lightIndex < pointLightCount; lightIndex += COMPUTE_SHADER_CLUSTER_GROUP_SIZE)
	{
		//PointLight light = pointLight[lightIndex];

		// Cull: point light sphere vs cluster frustum
		bool inFrustum = true;

		float4 lightPositionView = mul(float4(pointLight[lightIndex].Position, 1.0f), gView);

		[unroll]
		for (uint i = 0; i < 6; ++i)
		{
			/*
			float4 lightPositionView = mul(float4(light.Position, 1.0f), gView);
			float d = dot(frustumPlanes[i], lightPositionView);
			inFrustum = inFrustum && (d >= -light.Range);
			*/
			inFrustum = inFrustum && (lightPositionView.w >= 0);
		}

		/*
		[branch]
		if (inFrustum)
		{
			// Append light to list
			// Compaction might be better if we expect a lot of lights
			uint listIndex;
			InterlockedAdd(sClusterNumPointLights, 1, listIndex);
			sClusterPointLightIndices[listIndex] = lightIndex;
		}
		*/
		if (inFrustum)
		{
			test1++;
		}
    }

    GroupMemoryBarrierWithGroupSync();

	/*
	uint clusterZ = 0;
	for (clusterZ = 0; ((depth > DepthSlicing_16[clusterZ + 1]) && (clusterZ < CLUSTER_NUM_Z - 1)); clusterZ++)
	{
		;
	}
	
	//TODO: Output light list.

    uint numLights = sClusterNumPointLights;

    // Only process onscreen pixels (tiles can span screen edges)
	if (all(globalCoords < gRenderTargetSize.xy) && (linearDepth > 0.0f) && (clusterZ == groupId.z))
	{
#if VISUALIZE_CLUSTER_DISTRIBUTION
		float outputRGB = float(clusterZ) / 16.0f;
		gFramebuffer[globalCoords] = float4(outputRGB, outputRGB, outputRGB, 1.0f);
#elif VISUALIZE_CLUSTER_LIGHT_NUM
		float outputRGB = float(numLights) / 30.0f;
		gFramebuffer[globalCoords] = float4(outputRGB, outputRGB, outputRGB, 1.0f);
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

#if USE_CBDR
			// Point light.
			for (i = 0; i < numLights; i++)
			{
				shadowAmount = 1.f;
				float atten = Attenuate(pointLight[sClusterPointLightIndices[i]].Position, pointLight[sClusterPointLightIndices[i]].Range, worldPos);
				float lightIntensity = pointLight[sClusterPointLightIndices[i]].Intensity * atten;
				float3 toLight = normalize(pointLight[sClusterPointLightIndices[i]].Position - worldPos);
				float3 lightColor = pointLight[sClusterPointLightIndices[i]].Color.rgb;

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
		//*/

	//float test = (float)sClusterNumPointLights / 30.0f;
	float test = (float)test1 / 30.0f;
	float3 finalColor = float3(test, test, test);
	gFramebuffer[globalCoords] = float4(finalColor, 1.0f);
}


#endif



