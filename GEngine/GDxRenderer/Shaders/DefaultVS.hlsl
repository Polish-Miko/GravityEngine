
#include "Material.hlsli"
#include "ObjectCB.hlsli"
#include "MainPassCB.hlsli"
#include "HaltonSequence.hlsli"

struct Light
{
	float3 Strength;
	float FalloffStart; // point/spot light only
	float3 Direction;   // directional/spot light only
	float FalloffEnd;   // point/spot light only
	float3 Position;    // point light only
	float SpotPower;    // spot light only
};

struct VertexInput
{
	float3 pos		: POSITION;
	float2 uv		: TEXCOORD;
	float3 normal	: NORMAL;
	float3 tangent	: TANGENT;
};

struct VertexOutput
{
	float4	pos			: SV_POSITION;
	float2	uv			: TEXCOORD;
	float3	normal		: NORMAL;
	float3	tangent		: TANGENT;
	float3	worldPos	: POSITION0;
	float4	curPos		: POSITION1;
	float4	prevPos		: POSITION2;
	float	linearZ : LINEARZ;
	float4	shadowPos	: SHADOWPOS;
};

// Compute jittered view projection matrix.
float4x4 GetJitteredViewProj()
{
	uint subsampIndex = gFrameCount % SAMPLE_COUNT;

	float4x4 jitteredProj = gProj;
	jitteredProj[2][0] += Halton_2_3[subsampIndex].x * gInvRenderTargetSize.x * JitterDistance;
	jitteredProj[2][1] += Halton_2_3[subsampIndex].y * gInvRenderTargetSize.y * JitterDistance;

	float4x4 jitteredViewProj = mul(gView, jitteredProj);

	return jitteredViewProj;
}

float2 ProjectionConstants(float gNearZ, float gFarZ)
{
	float2 projectionConstants;
	projectionConstants.x = gFarZ / (gFarZ - gNearZ);
	projectionConstants.y = (-gFarZ * gNearZ) / (gFarZ - gNearZ);
	return projectionConstants;
}

float LinearZ(float4 outPosition)
{
	float2 projectionConstants = ProjectionConstants(gNearZ, gFarZ);
	float depth = outPosition.z / outPosition.w;
	float linearZ = projectionConstants.y / (depth - projectionConstants.x);
	return linearZ;
}

VertexOutput main(VertexInput input)
{
	VertexOutput output;

	MaterialData matData = gMaterialData[gMaterialIndex];

	float4x4 jitteredViewProj = GetJitteredViewProj();

	float4 worldPos = mul(float4(input.pos, 1.0f), gWorld);
	float4 prevWorldPos = mul(float4(input.pos, 1.0f), gPrevWorld);
	output.curPos = mul(worldPos, gViewProj);
	output.prevPos = mul(prevWorldPos, gPrevViewProj);
	float4 texC = float4(input.uv, 0.0f, 1.0f);
	output.pos = mul(worldPos, jitteredViewProj);
	output.uv = mul(texC, matData.MatTransform).xy;
	output.normal = normalize(mul(input.normal, (float3x3)gInvTransWorld));
	output.tangent = normalize(mul(input.tangent, (float3x3)gInvTransWorld));
	output.worldPos = mul(float4(input.pos, 1.0f), gWorld).xyz;
	output.linearZ = LinearZ(output.pos);
	output.shadowPos = mul(float4(input.pos, 1.0f), gShadowTransform);
	//output.ssaoPos = mul(worldPos, gViewProjTex);
	return output;
}