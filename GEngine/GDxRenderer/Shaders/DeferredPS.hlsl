#include "Lighting.hlsli"
#include "Material.hlsli"
#include "ObjectCB.hlsli"

struct VertexOutput
{
	float4	pos			: SV_POSITION;
	float2	uv			: TEXCOORD;
	float3	normal		: NORMAL;
	float3	tangent		: TANGENT;
	float3	worldPos	: POSITION0;
	float4	curPos		: POSITION1;
	float4	prevPos		: POSITION2;
	float	linearZ		: LINEARZ;
	float4	shadowPos	: SHADOWPOS;
};

struct PixelOutput
{
	float4	albedo						: SV_TARGET0;
	float4	normal						: SV_TARGET1;
	float4	worldPos					: SV_TARGET2;
	float2	velocity					: SV_TARGET3;
	float4	occlusionRoughnessMetallic	: SV_TARGET4;
};

Texture2D gTextureMaps[MAX_TEXTURE_NUM] : register(t0);
SamplerState Sampler	   : register(s0);

float3 calculateNormalFromMap(float3 normalFromTexture, float3 normal, float3 tangent)
{
	float3 unpackedNormal = normalFromTexture * 2.0f - 1.0f;
	float3 N = normal;
	float3 T = normalize(tangent - N * dot(tangent, N));
	float3 B = cross(N, T);
	float3x3 TBN = float3x3(T, B, N);
	return normalize(mul(unpackedNormal, TBN));
}

PixelOutput main(VertexOutput input)// : SV_TARGET
{
	MaterialData matData = gMaterialData[gMaterialIndex];

	uint diffuseMapIndex = matData.TextureIndex[0];
	uint normalMapIndex = matData.TextureIndex[1];
	uint OrmMapIndex = matData.TextureIndex[2];
 
	float3 albedoFromTexture = gTextureMaps[diffuseMapIndex].Sample(Sampler, input.uv).rgb;
	float3 normalFromTexture = gTextureMaps[normalMapIndex].Sample(Sampler, input.uv).rgb;
	float3 ormFromTexture = gTextureMaps[OrmMapIndex].Sample(Sampler, input.uv).rgb;
	//if (matData.TextureSrgb[0] == 1)
		//albedoFromTexture = pow(albedoFromTexture, 2.2f);
	//if (matData.TextureSrgb[1] == 1)
		//normalFromTexture = pow(normalFromTexture, 2.2f);
	//if (matData.TextureSrgb[2] == 1)
		//ormFromTexture = pow(ormFromTexture, 2.2f);

	float4 prevPos = input.prevPos;
	prevPos = prevPos / prevPos.w;
	prevPos.xy = prevPos.xy / float2(2.0f, -2.0f) + float2(0.5f, 0.5f);//negate Y because world coord and tex coord have different Y axis.
	float4 curPos = input.curPos;
	curPos = curPos / curPos.w;
	curPos.xy = curPos.xy / float2(2.0f, -2.0f) + float2(0.5f, 0.5f);//negate Y because world coord and tex coord have different Y axis.

	float3 normal = calculateNormalFromMap(normalFromTexture, normalize(input.normal), input.tangent);
	PixelOutput output;
	output.albedo = float4(albedoFromTexture, 1.0f);;
	output.normal = float4(normalize(normal), 1.0f);
	output.worldPos = float4(input.worldPos, 0.0f);
	output.velocity = float2(curPos.x - prevPos.x, curPos.y - prevPos.y);
	//output.velocity = float2(prevPos.x, prevPos.y);
	float roughness = ormFromTexture.g;
	float metal = ormFromTexture.b;
	output.occlusionRoughnessMetallic = float4(0, roughness, metal, 0);
	output.albedo.a = input.linearZ;
	return output;
}