
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"

#define MAX_SCENE_OBJECT_NUM 2048

#define MAX_STEP 200
#define ACCUM_DENSITY 0.1f
#define RAY_MARCH_DIS 10.0f

#define DEBUG 0

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

cbuffer cbSDF : register(b0)
{
	uint gSceneObjectNum;
	//uint gPad0;
	//uint gPad1;
	//uint gPad2;
};

StructuredBuffer<MeshSdfDescriptor> gMeshSdfDescriptors : register(t0);
StructuredBuffer<SceneObjectSdfDescriptor> gSceneObjectSdfDescriptors : register(t1);

Texture3D gSdfTextures[MAX_SCENE_OBJECT_NUM] : register(t0, space1);

SamplerState			basicSampler	: register(s0);
SamplerComparisonState	shadowSampler	: register(s1);

float3 ReconstructWorldPos(float2 uv, float depth)
{
	float ndcX = uv.x * 2 - 1;
	float ndcY = 1 - uv.y * 2; // remember to flip y!!!
	float4 viewPos = mul(float4(ndcX, ndcY, depth, 1.0f), gInvProj);
	viewPos = viewPos / viewPos.w;
	return mul(viewPos, gInvView).xyz;
}

float4 main(VertexToPixel pIn) : SV_TARGET
{

#if !DEBUG
	float3 origin = ReconstructWorldPos(pIn.uv, NEAR_Z_NORM);
	float3 dest = ReconstructWorldPos(pIn.uv, 0.5f);
	float3 dir = normalize(dest - origin) * RAY_MARCH_DIS;

	float alpha = 0.0f;
	for (int i = 0; i < gSceneObjectNum; i++)
	{
		int sdfInd = gSceneObjectSdfDescriptors[i].SdfIndex;

		float3 objDir = mul(dir, (float3x3)(gSceneObjectSdfDescriptors[i].objInvWorld_IT));
		float3 objOrigin = mul(origin, gSceneObjectSdfDescriptors[i].objInvWorld);
		float3 currPos = objOrigin;

		// March.
		float rcpHalfExtent = rcp(gMeshSdfDescriptors[sdfInd].HalfExtent);
		for (int step = 0; step < MAX_STEP; step++)
		{
			currPos += objDir;
			float3 pos = (currPos * rcpHalfExtent) * 0.5f + 0.5f;
			if (pos.x < 0 || pos.x > 1 ||
				pos.y < 0 || pos.y > 1 ||
				pos.z < 0 || pos.z > 1)
				continue;

			// Accumulate the distance as a density.
			float dist = gSdfTextures[sdfInd].Sample(basicSampler, pos).r;
			float dens = saturate(-dist) * ACCUM_DENSITY;

			alpha += saturate(dens);
		}
	}

	return float4(alpha, alpha, alpha, 1.0f);
#else

	float3 origin = ReconstructWorldPos(pIn.uv, NEAR_Z_NORM);
	float3 dest = ReconstructWorldPos(pIn.uv, 0.5f);
	float3 dir = normalize(dest - origin);

	float3 coord = float3(pIn.uv, 0.0f);
	float test = gSdfTextures[0].Sample(basicSampler, coord).r / 30.0f;

	return float4(origin, 1.0f);

#endif

}

