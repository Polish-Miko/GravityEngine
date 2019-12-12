
#include "StaticSamplers.hlsli"
#include "ShaderUtil.hlsli"
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

static const float F0_NON_METAL = 0.04f;

// Input textures.
Texture2D gDepthTexture							: register(t0);
Texture2D gNormalTexture						: register(t1);
Texture2D gOrmTexture							: register(t2);
Texture2D gGfTexture							: register(t3);
Texture2D gOcclusionTexture						: register(t4);
Texture2D gSceneColor							: register(t5);
Texture2D gAmbientSpecular						: register(t6);
Texture2D gSsrColor								: register(t7);
Texture2D gAlbedoTexture						: register(t8);

float3 ReconstructWorldPos(float2 uv, float depth)
{
	float ndcX = uv.x * 2 - 1;
	float ndcY = 1 - uv.y * 2; // Remember to flip y!!!
	float4 viewPos = mul(float4(ndcX, ndcY, depth, 1.0f), gUnjitteredInvProj);
	viewPos = viewPos / viewPos.w;
	return mul(viewPos, gInvView).xyz;
}

float2 BrdfLUT(float NdotV, float roughness)
{
	float2 uv = float2(NdotV, roughness);
	uv = clamp(uv, float2(0.0f, 0.0f), float2(0.99f, 0.99f));
	return gGfTexture.Sample(basicSampler, uv).rg;
}

float3 FresnelSchlickRoughness(float3 v, float3 n, float3 f0, float roughness)
{
	float NdotV = max(dot(v, n), 0.0f);
	float r1 = 1.0f - roughness;
	return f0 + (max(float3(r1, r1, r1), f0) - f0) * pow(1 - NdotV, 5.0f);
}

half3 PreintegratedDGF_LUT(half3 albedo, half3 normal, half metalness, half roughness, half NdotV, half3 V)
{
	float2 brdf = BrdfLUT(NdotV, roughness);
	float3 f0 = lerp(F0_NON_METAL.rrr, albedo.rgb, metalness);

	float3 kS = FresnelSchlickRoughness(V, normal, f0, roughness);
	float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
	kD *= (1.0f - metalness);

	return (kS * brdf.x + float3(brdf.y, brdf.y, brdf.y));
}

// Velocity texture setup
float4 main(VertexToPixel i) : SV_Target
{
	half2 uv = i.uv.xy;
	float2 jitteredUV = uv + gJitter;

	float depth = gDepthTexture.SampleLevel(linearClampSampler, jitteredUV, 0.0f).r;
	float3 albedo = gAlbedoTexture.SampleLevel(linearClampSampler, jitteredUV, 0.0f).rgb;
	float3 orm = gOrmTexture.SampleLevel(linearClampSampler, jitteredUV, 0.0f).rgb;
	float roughness = clamp(orm.g, 0.02f, 1.0f);
	float metalness = orm.b;
	float3 worldNormal = gNormalTexture.SampleLevel(linearClampSampler, jitteredUV, 0.0f).rgb;
	float3 worldPos = ReconstructWorldPos(uv, depth);
	float3 viewDir = normalize(worldPos - gEyePosW);
	
	half NdotV = saturate(dot(worldNormal, -viewDir));
	half3 PreintegratedGF = PreintegratedDGF_LUT(albedo, worldNormal, metalness, roughness, NdotV, -viewDir);

	half ReflectionOcclusion = saturate(gOcclusionTexture.SampleLevel(linearClampSampler, uv, 0.0f).g);
	//ReflectionOcclusion = ReflectionOcclusion == 0.5 ? 1 : ReflectionOcclusion;
	//half ReflectionOcclusion = 1;

	half3 SceneColor = gSceneColor.SampleLevel(linearClampSampler, jitteredUV, 0.0f).rgb;
	half4 AmbientSpecularColor = gAmbientSpecular.SampleLevel(linearClampSampler, jitteredUV, 0.0f);

	half4 SSRColor = gSsrColor.SampleLevel(linearClampSampler, uv, 0.0f);
	half SSRMask = SSRColor.a * SSRColor.a;
	//half SSRMask = lerp(SSRColor.a * SSRColor.a, 0.0f, smoothstep(0.3f, 0.5f, roughness));
	half3 ReflectionColor = (AmbientSpecularColor.rgb * (-SSRMask) + SSRColor.rgb * PreintegratedGF * SSRMask) * ReflectionOcclusion;

	return float4(SceneColor + ReflectionColor, 1.0f);
}

