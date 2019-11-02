
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"

#define MAX_SCENE_OBJECT_NUM 2048

#define MAX_STEP 200
#define MAX_DISTANCE 2000.0f
#define ACCUM_DENSITY 0.1f
#define STEP_LENGTH 10.0f
#define MIN_STEP_LENGTH 5.0f

#define CONE_COTANGENT 8.0f

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

Texture2D gDepthBuffer				: register(t2);

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

float3 ReconstructWorldPos(float2 uv, float depth)
{
	float ndcX = uv.x * 2 - 1;
	float ndcY = 1 - uv.y * 2; // remember to flip y!!!
	float4 viewPos = mul(float4(ndcX, ndcY, depth, 1.0f), gInvProj);
	viewPos = viewPos / viewPos.w;
	return mul(viewPos, gInvView).xyz;
}

float main(VertexToPixel pIn) : SV_TARGET
{

	float depthBuffer = gDepthBuffer.Sample(basicSampler, pIn.uv).r;
	float3 origin = ReconstructWorldPos(pIn.uv, depthBuffer);
	float3 dir = -normalize(gMainDirectionalLightDir.xyz) * STEP_LENGTH;

	float shadow = 1.0f;
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

		/*
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
		*/
	}
	
	return shadow;

}

