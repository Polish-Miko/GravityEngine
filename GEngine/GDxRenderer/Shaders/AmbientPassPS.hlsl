
#include "Lighting.hlsli"

#define PREFILTER_MIP_LEVEL 5

//G-Buffer
Texture2D gAlbedoTexture			: register(t0);
Texture2D gNormalTexture			: register(t1);
Texture2D gWorldPosTexture			: register(t2);
Texture2D gVelocityTexture			: register(t3);
Texture2D gOrmTexture				: register(t4);

TextureCube skyIrradianceTexture	: register(t5);
Texture2D	brdfLUTTexture			: register(t6);
TextureCube skyPrefilterTexture[PREFILTER_MIP_LEVEL]	: register(t7);

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

SamplerState			basicSampler	: register(s0);
SamplerComparisonState	shadowSampler	: register(s1);

float3 PrefilteredColor(float3 viewDir, float3 normal, float roughness)
{
	float roughnessLevel = roughness * PREFILTER_MIP_LEVEL;
	int fl = floor(roughnessLevel);
	int cl = ceil(roughnessLevel);
	float3 R = reflect(-viewDir, normal);
	float3 flSample = skyPrefilterTexture[fl].Sample(basicSampler, R).rgb;
	float3 clSample = skyPrefilterTexture[cl].Sample(basicSampler, R).rgb;
	float3 prefilterColor = lerp(flSample, clSample, (roughnessLevel - fl));
	return prefilterColor;
}

float2 BrdfLUT(float3 normal, float3 viewDir, float roughness)
{
	float NdotV = dot(normal, viewDir);
	NdotV = max(NdotV, 0.0f);
	float2 uv = float2(NdotV, roughness);
	return brdfLUTTexture.Sample(basicSampler, uv).rg;
}

float4 main(VertexToPixel pIn) : SV_TARGET
{
	float4 packedAlbedo = gAlbedoTexture.Sample(basicSampler, pIn.uv);

	// if alpha channel is nearly zero, clip this pixel.
	//clip(packedAlbedo.a - 0.05f);

	float3 albedo = packedAlbedo.rgb;
	float3 normal = gNormalTexture.Sample(basicSampler, pIn.uv).rgb;
	float3 worldPos = gWorldPosTexture.Sample(basicSampler, pIn.uv).rgb;
	float roughness = gOrmTexture.Sample(basicSampler, pIn.uv).g;
	float metal = gOrmTexture.Sample(basicSampler, pIn.uv).b;
	float shadowAmount = 1.f;

	//clamp roughness
	roughness = max(ROUGHNESS_CLAMP, roughness);

	float3 viewDir = normalize(cameraPosition - worldPos);
	float3 prefilter = PrefilteredColor(viewDir, normal, roughness);
	float2 brdf = BrdfLUT(normal, viewDir, roughness);
	float3 irradiance = skyIrradianceTexture.Sample(basicSampler, normal).rgb;

	float3 finalColor = AmbientPBR(normalize(normal), worldPos,
		cameraPosition, roughness, metal, albedo,
		irradiance, prefilter, brdf, shadowAmount);


	return float4(finalColor, 1.0f);
}