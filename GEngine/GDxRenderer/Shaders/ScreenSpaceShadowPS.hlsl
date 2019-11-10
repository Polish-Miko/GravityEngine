
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"

#define MAX_SCENE_OBJECT_NUM 2048

#define MAX_STEP 200
#define MAX_DISTANCE 2000.0f
#define ACCUM_DENSITY 0.1f
#define STEP_LENGTH 10.0f
#define MIN_STEP_LENGTH 5.0f

#define CONE_COTANGENT 8.0f

#define DEBUG_CASCADE_RANGE 0
#define DEBUG_SHADOWMAP 0

#define PCSS_SOFTNESS 1.5f

#define BLOCK_SEARCH_SAMPLE_NUM 8
#define BLOCK_SEARCH_SAMPLING_UV_RADIUS 0.003f

#define PCF_SAMPLE_NUM 32
#define PCF_TEMPORAL_FRAME_COUNT 1
#define PCF_SAMPLING_UV_RADIUS 0.025f

static const float PI = 3.14159265359f;

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

struct MeshSdfDescriptor
{
	float HalfExtent;
	float Radius;
	int Resolution;
};

struct SceneObjectSdfDescriptor
{
	float4x4 objWorld;
	float4x4 objInvWorld;
	float4x4 objInvWorld_IT;
	int SdfIndex;
};

StructuredBuffer<MeshSdfDescriptor> gMeshSdfDescriptors : register(t0);
StructuredBuffer<SceneObjectSdfDescriptor> gSceneObjectSdfDescriptors : register(t1);

Texture2D gDepthBuffer					: register(t2);

Texture2D gBlueNoise					: register(t3);

Texture2D gShadowMap[SHADOW_CASCADE_NUM]	: register(t4);

Texture3D gSdfTextures[MAX_SCENE_OBJECT_NUM] : register(t0, space1);

SamplerState			basicSampler	: register(s0);
SamplerComparisonState	shadowSampler	: register(s1);

cbuffer cbSDF : register(b0)
{
	uint gSceneObjectNum;
	//uint gPad0;
	//uint gPad1;
	//uint gPad2;
};

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

	[unroll]
	for (int cascadeIndex = 0; cascadeIndex < SHADOW_CASCADE_NUM; cascadeIndex++)
	{
		float sampledDepth = gDepthBuffer.SampleLevel(basicSampler, pIn.uv, 0).r;
		float3 worldPos = ReconstructWorldPos(pIn.uv, sampledDepth);
		float4 shadowPos = mul(float4(worldPos, 1.0f), gShadowTransform[cascadeIndex]);
		shadowPos /= shadowPos.w;
		float2 dz_duv = DepthGradient(shadowPos.xy, shadowPos.z);

		if (shadowPos.x < 0.0f ||
			shadowPos.x > 1.0f ||
			shadowPos.y < 0.0f ||
			shadowPos.y > 1.0f)
			continue;

#if !DEBUG_CASCADE_RANGE

		shadow = PCSS(shadowPos.xyz, cascadeIndex, dz_duv);

#else
		shadow = (float)cascadeIndex / SHADOW_CASCADE_NUM;
#endif

		break;
	}

#if DEBUG_SHADOWMAP
	shadow = gShadowMap[0].SampleLevel(basicSampler, pIn.uv, 0).r;
#endif

	return shadow;

	//return gShadowMap[0].Sample(basicSampler, pIn.uv).r;
	//return 1.0f;

	/*
	float depthBuffer = gDepthBuffer.Sample(basicSampler, pIn.uv).r;
	float3 origin = ReconstructWorldPos(pIn.uv, depthBuffer);
	float3 dir = -normalize(gMainDirectionalLightDir.xyz) * STEP_LENGTH;

	for (int i = 0; i < gSceneObjectNum; i++)
	{
		int sdfInd = gSceneObjectSdfDescriptors[i].SdfIndex;

		float3 objOrigin = mul(float4(origin, 1.0f), gSceneObjectSdfDescriptors[i].objInvWorld).xyz;
		float3 currPos = objOrigin;

		float3 objDir = mul(dir, (float3x3)(gSceneObjectSdfDescriptors[i].objInvWorld_IT));
		float stepLength = length(objDir);
		objDir = normalize(objDir);

		float totalDis = 0.0f;

		// March.
		float rcpHalfExtent = rcp(gMeshSdfDescriptors[sdfInd].HalfExtent);
		for (int step = 0; step < MAX_STEP && totalDis < MAX_DISTANCE; step++)
		{
			currPos = objOrigin + objDir * totalDis;

			float3 pos = (currPos * rcpHalfExtent) * 0.5f + 0.5f;

			if (pos.x < 0 || pos.x > 1 ||
				pos.y < 0 || pos.y > 1 ||
				pos.z < 0 || pos.z > 1)
			{
				totalDis += stepLength;
				continue;
			}

			float dist = gSdfTextures[sdfInd].Sample(basicSampler, pos).r;

			dist = clamp(dist, MIN_STEP_LENGTH, dist + 1);
			totalDis += dist;
			shadow = min(shadow, saturate(CONE_COTANGENT * dist / totalDis));
		}

		float prevDist = 1e10;

		float rcpHalfExtent = rcp(gMeshSdfDescriptors[sdfInd].HalfExtent);
		for (int step = 0; step < MAX_STEP && totalDis < MAX_DISTANCE; step++)
		{
			currPos = objOrigin + objDir * totalDis;

			float3 pos = (currPos * rcpHalfExtent) * 0.5f + 0.5f;

			if (pos.x < 0 || pos.x > 1 ||
				pos.y < 0 || pos.y > 1 ||
				pos.z < 0 || pos.z > 1)
			{
				totalDis += stepLength;
				continue;
			}

			float dist = gSdfTextures[sdfInd].Sample(basicSampler, pos).r;

			float y = dist * dist / (2.0 * prevDist);
			float d = sqrt(dist * dist - y * y);

			totalDis += clamp(dist, MIN_STEP_LENGTH, dist + 1);

			shadow = min(shadow, 10.0 * d / max(0.0, totalDis - y));
		}
	}
	
	return shadow;

	*/
}

