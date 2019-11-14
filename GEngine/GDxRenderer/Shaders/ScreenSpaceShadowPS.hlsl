
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"
#include "SdfDescriptors.hlsli"

#define MAX_SCENE_OBJECT_NUM 2048

#define MAX_STEP 64
#define MAX_DISTANCE 3000.0f
#define ACCUM_DENSITY 0.1f
#define STEP_LENGTH 10.0f
#define MIN_STEP_LENGTH (1.0f / (4 * MAX_STEP))

#define CONE_COTANGENT 10.0f
#define CONE_TANGENT (1 / CONE_COTANGENT)

#define DEBUG_CASCADE_RANGE 0
#define DEBUG_SHADOWMAP 0

#define PCSS_SOFTNESS 1.5f

#define BLOCK_SEARCH_SAMPLE_NUM 8
#define BLOCK_SEARCH_SAMPLING_UV_RADIUS 0.003f

#define PCF_SAMPLE_NUM 32
#define PCF_TEMPORAL_FRAME_COUNT 1
#define PCF_SAMPLING_UV_RADIUS 0.025f

#define RAY_START_OFFSET 3.0f

#define MIN_SPHERE_RADIUS 0.4f
#define MAX_SPHERE_RADIUS 100.0f

#define USE_HARD_SHADOW 1

static const float PI = 3.14159265359f;

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

struct SdfList
{
	uint NumSdf;
	uint SdfObjIndices[MAX_GRID_SDF_NUM];
};

Texture2D gDepthBuffer					: register(t2);

Texture2D gBlueNoise					: register(t3);

StructuredBuffer<SdfList> gSdfList : register(t4);

Texture2D gShadowMap[SHADOW_CASCADE_NUM]	: register(t5);

Texture3D gSdfTextures[MAX_SCENE_OBJECT_NUM] : register(t0, space1);

SamplerState			basicSampler	: register(s0);
SamplerComparisonState	shadowSampler	: register(s1);

//-------------------------------------------------------------------------------------------------
// Util.
//-------------------------------------------------------------------------------------------------

float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ReconstructWorldPos(float2 uv, float depth)
{
	float ndcX = uv.x * 2 - 1;
	float ndcY = 1 - uv.y * 2; // remember to flip y!!!
	float4 viewPos = mul(float4(ndcX, ndcY, depth, 1.0f), gInvProj);
	viewPos = viewPos / viewPos.w;
	return mul(viewPos, gInvView).xyz;
}

inline float2 RandomDirection(int i, int n, int frameCount)
{
	int frameIndex = gFrameCount % frameCount;
	int frameOffset = frameIndex * frameCount;
	float2 uv = Hammersley(i + frameOffset, n * frameCount);
	float random = gBlueNoise.SampleLevel(basicSampler, uv, 0).r;
	float randomAngle = random * 2.0f * PI;
	return float2(cos(randomAngle), sin(randomAngle));
}

float LinearDepth(float depth)
{
	return (depth * NEAR_Z) / (FAR_Z - depth * (FAR_Z - NEAR_Z));
}

float ReverseLinearDepth(float linearDepth)
{
	return (linearDepth * FAR_Z) / (NEAR_Z + linearDepth * (FAR_Z - NEAR_Z));
}

float ClipZToViewZ(float clipZ)
{
	return (clipZ * (LIGHT_FAR_Z - LIGHT_NEAR_Z) + LIGHT_FAR_Z + LIGHT_NEAR_Z) / 2.0f;
}

// Derivatives of light-space depth with respect to texture coordinates
float2 DepthGradient(float2 uv, float z)
{
	float2 dz_duv = 0;

	float3 duvdist_dx = ddx(float3(uv, z));
	float3 duvdist_dy = ddy(float3(uv, z));

	dz_duv.x = duvdist_dy.y * duvdist_dx.z;
	dz_duv.x -= duvdist_dx.y * duvdist_dy.z;

	dz_duv.y = duvdist_dx.x * duvdist_dy.z;
	dz_duv.y -= duvdist_dy.x * duvdist_dx.z;

	float det = (duvdist_dx.x * duvdist_dy.y) - (duvdist_dx.y * duvdist_dy.x);
	dz_duv /= det;

	return dz_duv;
}

/*
* Clips a ray to an AABB.  Does not handle rays parallel to any of the planes.
*
* @param RayOrigin - The origin of the ray in world space.
* @param RayEnd - The end of the ray in world space.
* @param BoxMin - The minimum extrema of the box.
* @param BoxMax - The maximum extrema of the box.
* @return - Returns the closest intersection along the ray in x, and furthest in y.
*			If the ray did not intersect the box, then the furthest intersection <= the closest intersection.
*			The intersections will always be in the range [0,1], which corresponds to [RayOrigin, RayEnd] in worldspace.
*			To find the world space position of either intersection, simply plug it back into the ray equation:
*			WorldPos = RayOrigin + (RayEnd - RayOrigin) * Intersection;
*/
float2 LineBoxIntersect(float3 RayOrigin, float3 RayEnd, float3 BoxMin, float3 BoxMax)
{
	float3 InvRayDir = 1.0f / (RayEnd - RayOrigin);

	//find the ray intersection with each of the 3 planes defined by the minimum extrema.
	float3 FirstPlaneIntersections = (BoxMin - RayOrigin) * InvRayDir;
	//find the ray intersection with each of the 3 planes defined by the maximum extrema.
	float3 SecondPlaneIntersections = (BoxMax - RayOrigin) * InvRayDir;
	//get the closest of these intersections along the ray
	float3 ClosestPlaneIntersections = min(FirstPlaneIntersections, SecondPlaneIntersections);
	//get the furthest of these intersections along the ray
	float3 FurthestPlaneIntersections = max(FirstPlaneIntersections, SecondPlaneIntersections);

	float2 BoxIntersections;
	//find the furthest near intersection
	BoxIntersections.x = max(ClosestPlaneIntersections.x, max(ClosestPlaneIntersections.y, ClosestPlaneIntersections.z));
	//find the closest far intersection
	BoxIntersections.y = min(FurthestPlaneIntersections.x, min(FurthestPlaneIntersections.y, FurthestPlaneIntersections.z));
	//clamp the intersections to be between RayOrigin and RayEnd on the ray
	return saturate(BoxIntersections);
}

float SampleMeshDistanceField(int sdfInd, float3 uv)
{
	float dist = gSdfTextures[sdfInd].SampleLevel(basicSampler, uv, 0).r;

	return dist;
}

float SampleMeshDistanceField(int sdfInd, float SdfScale, float3 uv)
{
	float dist = gSdfTextures[sdfInd].SampleLevel(basicSampler, uv, 0).r;
	dist = (dist - 0.5f) * SdfScale;

	return dist;
}

//-------------------------------------------------------------------------------------------------
// Blocker Search.
//-------------------------------------------------------------------------------------------------

float BlockerSearch(float3 shadowPos, int shadowMapIndex, float2 dz_duv)
{
	float receiverDistance = shadowPos.z;

#if USE_REVERSE_Z
	receiverDistance = 1.0f - receiverDistance;
#endif

	int blockers = 0;
	float avgBlockerDistance = 0;
	float searchSize = PCSS_SOFTNESS * BLOCK_SEARCH_SAMPLING_UV_RADIUS / pow(2.0f, shadowMapIndex);// *saturate(receiverDistance - 0.02f) / receiverDistance;

	for (int i = 0; i < BLOCK_SEARCH_SAMPLE_NUM; i++)
	{
		float2 uvOffset = (Hammersley(i, BLOCK_SEARCH_SAMPLE_NUM) * 2.0f - 1.0f) * searchSize;// RandomDirection(i / float(BLOCK_SEARCH_SAMPLE_NUM)) * searchSize;
		float2 sampleUV = shadowPos.xy + uvOffset;
		float z = gShadowMap[shadowMapIndex].SampleLevel(basicSampler, sampleUV, 0).r;

		float biasedZ = shadowPos.z + dot(dz_duv, uvOffset);

#if USE_REVERSE_Z
		if (z > biasedZ)
#else
		if (z < biasedZ)
#endif
		{
			blockers++;
			avgBlockerDistance += z;
		}
	}

	if (blockers > 0)
		return avgBlockerDistance / blockers;
	else
		return -1.0f;
}

//-------------------------------------------------------------------------------------------------
// PCF Filtering.
//-------------------------------------------------------------------------------------------------

float PCF(float3 shadowPos, int shadowMapIndex, float uvRadius, float2 dz_duv)
{
	float sum = 0.0f;
	for (int i = 0; i < PCF_SAMPLE_NUM; i++)
	{
		//float2 uvOffset = RandomDirection(i, PCF_SAMPLE_NUM, PCF_TEMPORAL_FRAME_COUNT) * uvRadius;
		int frameIndex = gFrameCount % PCF_TEMPORAL_FRAME_COUNT;
		int frameOffset = frameIndex * PCF_TEMPORAL_FRAME_COUNT;
		float2 uvOffset = (Hammersley(i + frameOffset, PCF_SAMPLE_NUM * PCF_TEMPORAL_FRAME_COUNT) * 2.0f - 1.0f) * uvRadius;// RandomDirection(i / float(PCF_SAMPLE_NUM)) * uvRadius;
		float2 sampleUV = shadowPos.xy + uvOffset;
		float z = gShadowMap[shadowMapIndex].SampleLevel(basicSampler, sampleUV, 0).r;

		float biasedZ = shadowPos.z + dot(dz_duv, uvOffset);

#if USE_REVERSE_Z
		sum += (z < biasedZ) ? 1 : 0;
#else
		sum += (z > biasedZ) ? 1 : 0;
#endif
	}
	return sum / PCF_SAMPLE_NUM;
}

//-------------------------------------------------------------------------------------------------
// PCSS.
//-------------------------------------------------------------------------------------------------

float PCSS(float3 shadowPos, int shadowMapIndex, float2 dz_duv)
{
	// blocker search.
	float blockerDistance = BlockerSearch(shadowPos, shadowMapIndex, dz_duv);

	if (blockerDistance == -1.0f)
		return 1.0f;

	// penumbra estimation.
	blockerDistance = ClipZToViewZ(blockerDistance);
	float receiverDistance = ClipZToViewZ(shadowPos.z);
	float penumbraWidth = (receiverDistance - blockerDistance) / blockerDistance;

	// percentage-close filtering.
	float uvRadius = penumbraWidth * PCF_SAMPLING_UV_RADIUS * PCSS_SOFTNESS / pow(2.0f, shadowMapIndex);
	return PCF(shadowPos, shadowMapIndex, uvRadius, dz_duv);
}

//-------------------------------------------------------------------------------------------------
// Main.
//-------------------------------------------------------------------------------------------------

float main(VertexToPixel pIn) : SV_TARGET
{
	float shadow = 1.0f;
	bool cascaded = false;

	float sampledDepth = gDepthBuffer.SampleLevel(basicSampler, pIn.uv, 0).r;
	float3 worldPos = ReconstructWorldPos(pIn.uv, sampledDepth);

	//-----------------------------------------CSM/PCSS------------------------------------------------
	[unroll]
	for (int cascadeIndex = 0; cascadeIndex < SHADOW_CASCADE_NUM; cascadeIndex++)
	{
		float4 shadowPos = mul(float4(worldPos, 1.0f), gShadowTransform[cascadeIndex]);
		shadowPos /= shadowPos.w;
		float2 dz_duv = DepthGradient(shadowPos.xy, shadowPos.z);

		if (shadowPos.x < 0.0f ||
			shadowPos.x > 1.0f ||
			shadowPos.y < 0.0f ||
			shadowPos.y > 1.0f)
			continue;

#if !DEBUG_CASCADE_RANGE

#if !USE_HARD_SHADOW
		shadow = PCSS(shadowPos.xyz, cascadeIndex, dz_duv);
#else
		float z = gShadowMap[cascadeIndex].SampleLevel(basicSampler, shadowPos.xy, 0).r;

#if USE_REVERSE_Z
		if (z > shadowPos.z)
#else
		if (z < shadowPos.z)
#endif
		{
			shadow = 0.0f;
		}
#endif

#else
		shadow = (float)cascadeIndex / SHADOW_CASCADE_NUM;
#endif

		cascaded = true;
		break;
	}

	//-----------------------------------------SDF Shadow----------------------------------------------

	[branch]
	if (!cascaded)
	{
		float3 lightDir = -normalize(gMainDirectionalLightDir.xyz);
		float3 opaqueWorldPos = worldPos;
		float3 rayStart = opaqueWorldPos + lightDir * RAY_START_OFFSET;
		float3 rayEnd = opaqueWorldPos + lightDir * MAX_DISTANCE;

		float2 tilePos = mul(float4(opaqueWorldPos, 1.0f), gSdfTileTransform).xy;
		int2 tileID = int2(floor(tilePos.x * SDF_GRID_NUM), floor((1.0f - tilePos.y) * SDF_GRID_NUM));
		int tileIndex = tileID.y * SDF_GRID_NUM + tileID.x;

		if (tileID.x < 0 || tileID.x >= SDF_GRID_NUM ||
			tileID.y < 0 || tileID.y >= SDF_GRID_NUM
			)
			return 1.0f;

		int objectNum = gSdfList[tileIndex].NumSdf;

		float minConeVisibility = 1.0f;

		[loop]
		for (int i = 0; i < objectNum; i++)
		{
			uint objIndex = gSdfList[tileIndex].SdfObjIndices[i];
			int sdfInd = gSceneObjectSdfDescriptors[objIndex].SdfIndex;

			float3 volumeRayStart = mul(float4(rayStart, 1.0f), gSceneObjectSdfDescriptors[objIndex].objInvWorld).xyz;
			float3 volumeRayEnd = mul(float4(rayEnd, 1.0f), gSceneObjectSdfDescriptors[objIndex].objInvWorld).xyz;
			float3 volumeRayDirection = volumeRayEnd - volumeRayStart;
			float volumeRayLength = length(volumeRayDirection);
			volumeRayDirection /= volumeRayLength;

			float halfExtent = gMeshSdfDescriptors[sdfInd].HalfExtent;
			float rcpHalfExtent = rcp(halfExtent);
#if USE_FIXED_POINT_SDF_TEXTURE
			float SdfScale = halfExtent * 2.0f * SDF_DISTANCE_RANGE_SCALE;
#endif

			float3 localPositionExtent = float3(halfExtent, halfExtent, halfExtent);
			float3 outOfBoxRange = float3(SDF_OUT_OF_BOX_RANGE, SDF_OUT_OF_BOX_RANGE, SDF_OUT_OF_BOX_RANGE);
			float2 intersectionTimes = LineBoxIntersect(volumeRayStart, volumeRayEnd, -localPositionExtent - outOfBoxRange, localPositionExtent + outOfBoxRange);

			[branch]
			if (intersectionTimes.x < intersectionTimes.y)
			{
				float sampleRayTime = intersectionTimes.x * volumeRayLength;

				uint stepIndex = 0;

				[loop]
				for (; stepIndex < MAX_STEP; stepIndex++)
				{
					float3 sampleVolumePosition = volumeRayStart + volumeRayDirection * sampleRayTime;
					float3 clampedSamplePosition = clamp(sampleVolumePosition, -localPositionExtent, localPositionExtent);
					float distanceToClamped = length(clampedSamplePosition - sampleVolumePosition);
					float3 volumeUV = (clampedSamplePosition * rcpHalfExtent) * 0.5f + 0.5f;
					float distanceField;
#if USE_FIXED_POINT_SDF_TEXTURE
					distanceField = SampleMeshDistanceField(sdfInd, SdfScale, volumeUV);
#else
					distanceField = SampleMeshDistanceField(sdfInd, volumeUV);
#endif
					distanceField += distanceToClamped;

					// Don't allow occlusion within an object's self shadow distance
					//float selfShadowVisibility = 1 - saturate(sampleRayTime * selfShadowScale);

					//float sphereRadius = clamp(TanLightAngle * sampleRayTime, VolumeMinSphereRadius, VolumeMaxSphereRadius);
					//float stepVisibility = max(saturate(distanceField / sphereRadius), selfShadowVisibility);
					float sphereRadius = CONE_TANGENT * sampleRayTime;
					float stepVisibility = saturate(distanceField / sphereRadius);

					minConeVisibility = min(minConeVisibility, stepVisibility);

					float stepDistance = max(abs(distanceField), MIN_STEP_LENGTH);
					sampleRayTime += stepDistance;

					// Terminate the trace if we are fully occluded or went past the end of the ray
					if (minConeVisibility < .01f ||
						sampleRayTime > intersectionTimes.y * volumeRayLength)
					{
						break;
					}
				}

				// Force to shadowed as we approach max steps
				minConeVisibility = min(minConeVisibility, (1 - stepIndex / (float)MAX_STEP));
			}

			if (minConeVisibility < .01f)
			{
				minConeVisibility = 0.0f;
				break;
			}
		}

		shadow = minConeVisibility;
	}

	//shadow = gSdfList[tileIndex].NumSdf / 2.0f;
	//shadow = (float)tileIndex / (SDF_GRID_NUM * SDF_GRID_NUM);

#if DEBUG_SHADOWMAP
	shadow = gShadowMap[1].SampleLevel(basicSampler, pIn.uv, 0).r;
#endif
	
	return shadow;

}

