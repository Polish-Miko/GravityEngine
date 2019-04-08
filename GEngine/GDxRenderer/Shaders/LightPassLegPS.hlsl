#include "Lighting.hlsli"

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

float4 calculateDirectionalLight(float3 normal, DirectionalLight light)
{
	float3 dirToLight = normalize(-light.Direction);
	float NdotL = dot(normal, dirToLight);
	NdotL = saturate(NdotL);
	return light.DiffuseColor * NdotL + light.AmbientColor;
}

float4 calculatePointLight(float3 normal, float3 worldPos, PointLight light)
{
	float3 dirToPointLight = normalize(light.Position - worldPos);
	float pointNdotL = dot(normal, dirToPointLight);
	pointNdotL = saturate(pointNdotL);
	return light.Color * pointNdotL;
}

//G-Buffer
Texture2D gAlbedoTexture			: register(t0);
Texture2D gNormalTexture			: register(t1);
Texture2D gWorldPosTexture			: register(t2);
Texture2D gRoughnessTexture			: register(t3);
Texture2D gMetalnessTexture			: register(t4);
Texture2D gDepth					: register(t7); //t5 reserved for this shaders output, t6 for ambient pass

//IBL
TextureCube skyIrradianceTexture	: register(t8);
Texture2D	brdfLUTTexture			: register(t9);
TextureCube skyPrefilterTexture		: register(t10);

//Shadow Dir Light
Texture2D gShadowMap				: register(t11);
Texture2D gShadowPos				: register(t12);

SamplerState			basicSampler	: register(s0);
SamplerComparisonState	shadowSampler	: register(s1);

float3 PrefilteredColor(float3 viewDir, float3 normal, float roughness)
{
	const float MAX_REF_LOD = 8.0f; // (mip level - 1) Mip Levels = 9
	float3 R = reflect(-viewDir, normal);
	return skyPrefilterTexture.SampleLevel(basicSampler, R, roughness * MAX_REF_LOD).rgb;
}

float2 BrdfLUT(float3 normal, float3 viewDir, float roughness)
{
	float NdotV = dot(normal, viewDir);
	NdotV = max(NdotV, 0.0f);
	float2 uv = float2(NdotV, roughness);
	return brdfLUTTexture.Sample(basicSampler, uv).rg;
}

float ShadowAmount(float4 shadowPos)
{
	float2 shadowUV = shadowPos.xy / shadowPos.w * 0.5f + 0.5f;
	shadowUV.y = 1.0f - shadowUV.y;
	float depthFromLight = shadowPos.z / shadowPos.w;
	float shadowAmount = gShadowMap.SampleCmpLevelZero(shadowSampler, shadowUV, depthFromLight);
	return shadowAmount;
}

float SampleShadowMap(float2 uv, float2 offsetUV, float2 shadowSizeInv, float depth)
{
	float2 sUV = uv + offsetUV * shadowSizeInv;
	return gShadowMap.SampleCmpLevelZero(shadowSampler, sUV, depth);
}

float SampleShadowMapOptimizedPCF(float4 shadowPos)
{
	float2 shadowMapSize = 4096; // TODO: send through CB
	float lightDepth = shadowPos.z / shadowPos.w;
	float2 uv = (shadowPos.xy / shadowPos.w * 0.5f + 0.5f);
	uv.y = 1.f - uv.y;
	uv = uv * shadowMapSize; //1 unit - 1 texel

	float2 shadowMapSizeInv = 1.0 / shadowMapSize;

	float2 base_uv;
	base_uv.x = floor(uv.x + 0.5);
	base_uv.y = floor(uv.y + 0.5);

	float s = (uv.x + 0.5 - base_uv.x);
	float t = (uv.y + 0.5 - base_uv.y);

	base_uv -= float2(0.5, 0.5);
	base_uv *= shadowMapSizeInv;


	float uw0 = (4 - 3 * s);
	float uw1 = 7;
	float uw2 = (1 + 3 * s);

	float u0 = (3 - 2 * s) / uw0 - 2;
	float u1 = (3 + s) / uw1;
	float u2 = s / uw2 + 2;

	float vw0 = (4 - 3 * t);
	float vw1 = 7;
	float vw2 = (1 + 3 * t);

	float v0 = (3 - 2 * t) / vw0 - 2;
	float v1 = (3 + t) / vw1;
	float v2 = t / vw2 + 2;


	float sum = 0;
	sum += uw0 * vw0 * SampleShadowMap(base_uv, float2(u0, v0), shadowMapSizeInv, lightDepth);
	sum += uw1 * vw0 * SampleShadowMap(base_uv, float2(u1, v0), shadowMapSizeInv, lightDepth);
	sum += uw2 * vw0 * SampleShadowMap(base_uv, float2(u2, v0), shadowMapSizeInv, lightDepth);

	sum += uw0 * vw1 * SampleShadowMap(base_uv, float2(u0, v1), shadowMapSizeInv, lightDepth);
	sum += uw1 * vw1 * SampleShadowMap(base_uv, float2(u1, v1), shadowMapSizeInv, lightDepth);
	sum += uw2 * vw1 * SampleShadowMap(base_uv, float2(u2, v1), shadowMapSizeInv, lightDepth);

	sum += uw0 * vw2 * SampleShadowMap(base_uv, float2(u0, v2), shadowMapSizeInv, lightDepth);
	sum += uw1 * vw2 * SampleShadowMap(base_uv, float2(u1, v2), shadowMapSizeInv, lightDepth);
	sum += uw2 * vw2 * SampleShadowMap(base_uv, float2(u2, v2), shadowMapSizeInv, lightDepth);

	return sum * 1.0f / 144;
}

float4 main(VertexToPixel pIn) : SV_TARGET
{
	int3 sampleIndices = int3(pIn.position.xy, 0);
	float4 packedAlbedo = gAlbedoTexture.Sample(basicSampler, pIn.uv);
	float3 albedo = packedAlbedo.rgb;
	float3 normal = gNormalTexture.Sample(basicSampler, pIn.uv).rgb;
	float3 worldPos = gWorldPosTexture.Sample(basicSampler, pIn.uv).rgb;
	float roughness = gRoughnessTexture.Sample(basicSampler, pIn.uv).r;
	float metal = gMetalnessTexture.Sample(basicSampler, pIn.uv).r;
	float4 shadowPos = gShadowPos.Sample(basicSampler, pIn.uv);
	float shadowAmount = SampleShadowMapOptimizedPCF(shadowPos);

	float3 viewDir = normalize(cameraPosition - worldPos);
	float3 prefilter = PrefilteredColor(viewDir, normal, roughness);
	float2 brdf = BrdfLUT(normal, viewDir, roughness);

	float3 specColor = lerp(F0_NON_METAL.rrr, albedo.rgb, metal);
	float3 irradiance = skyIrradianceTexture.Sample(basicSampler, normal).rgb;
	float3 finalColor = 0.f;

	for (int i = 0; i < dirLightCount; ++i)
	{
		finalColor = finalColor + DirLightPBR(dirLight[i], normalize(normal), worldPos,
			cameraPosition, roughness, metal, albedo, shadowAmount);
	}

	float3 totalColor = finalColor;
	return float4(totalColor, packedAlbedo.a);

}