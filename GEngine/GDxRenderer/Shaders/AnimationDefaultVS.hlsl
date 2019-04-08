#define MAX_BONES 128

struct VertexAnimatedInput
{
	float3 pos			: POSITION;
	float2 uv			: TEXCOORD;
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;
	uint4  skinIndices	: BLENDINDICES;
	float4 skinWeights	: BLENDWEIGHT;
};

struct VertexOutput
{
	float4 pos			: SV_POSITION;
	float2 uv			: TEXCOORD;
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;
	float3 worldPos		: POSITION;
	float  linearZ		: LINEARZ;
	float4 shadowPos	: SHADOWPOS;
};

cbuffer PerEntity : register(b0)
{
	float4x4 worldViewProjection;
	float4x4 world;
	float4x4 view;
	float4x4 projection;
	float4x4 shadowView;
	float4x4 shadowProjection;
};

cbuffer PerFrame : register(b1)
{
	float nearZ;
	float farZ;
	float2 lightPerspectiveValues;
};

cbuffer PerArmature: register(b2)
{
	matrix bones[128];
};

float2 ProjectionConstants(float nearZ, float farZ)
{
	float2 projectionConstants;
	projectionConstants.x = farZ / (farZ - nearZ);
	projectionConstants.y = (-farZ * nearZ) / (farZ - nearZ);
	return projectionConstants;
}

float LinearZ(float4 outPosition)
{
	float2 projectionConstants = ProjectionConstants(nearZ, farZ);
	float depth = outPosition.z / outPosition.w;
	float linearZ = projectionConstants.y / (depth - projectionConstants.x);
	return linearZ;
}

float4x4 SkinTransform(float4 weights, uint4 boneIndices)
{
	// Calculate the skin transform from up to four bones and weights
	float4x4 skinTransform = 
			bones[boneIndices.x] * weights.x +
			bones[boneIndices.y] * weights.y +
			bones[boneIndices.z] * weights.z +
			bones[boneIndices.w] * weights.w;
	return skinTransform;
}
//
//void SkinVertex(float4 weights, uint4 bones, inout float4 position, inout float3 normal)
//{
//	// If there are skin weights apply vertex skinning
//	if (weights.x != 0)
//	{
//		// Calculate the skin transform from up to four bones and weights
//		float4x4 skinTransform = bones[bones.x] * weights.x +
//			bones[bones.y] * weights.y +
//			bones[bones.z] * weights.z +
//			bones[bones.w] * weights.w;
//
//		// Apply skinning to vertex and normal
//		position = mul(position, skinTransform);
//
//		// We assume here that the skin transform includes only uniform scaling (if any)
//		normal = mul(normal, (float3x3)skinTransform);
//	}
//}

VertexOutput main(VertexAnimatedInput input)
{
	VertexOutput output;

	float4x4 skinTransform = SkinTransform(input.skinWeights, input.skinIndices);
	float4 position = float4(input.pos, 1.0f);
	//position = mul(position, worldViewProjection);
	if (input.skinWeights.x != 0)
	{
		position += mul(position, skinTransform);
	}

	float4x4 shadowVP = mul(mul(world, shadowView), shadowProjection);

	output.pos = mul(position, worldViewProjection);
	output.uv = input.uv;
	output.normal = normalize(mul(input.normal, (float3x3)world));
	output.tangent = normalize(mul(input.tangent, (float3x3)world));
	output.worldPos = mul(float4(input.pos, 1.0f), world).xyz;
	output.linearZ = LinearZ(output.pos);
	output.shadowPos = mul(float4(input.pos, 1.0f), shadowVP);
	return output;
}