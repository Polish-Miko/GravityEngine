#ifndef _LIGHTING_HLSLI
#define _LIGHTING_HLSLI

#include "ShaderDefinition.h"

#define CLAMP_ROUGHNESS
#define ROUGHNESS_CLAMP 0.02f

#define MAX_DIRECTIONAL_LIGHT_NUM 4
#define MAX_POINT_LIGHT_NUM 1024
#define MAX_SPOTLIGHT_NUM 1024

#define UNREAL_LIGHT_ATTENUATION 1

static const float MIN_ROUGHNESS = 0.0000001f;
static const float F0_NON_METAL = 0.04f;
static const float PI = 3.14159265359f;

struct SpotLight
{
	float4 Color;
	float4 Direction;
	float3 Position;
	float Range;
	float SpotlightAngle;
};

struct DirectionalLight
{
	float4 AmbientColor;
	float4 DiffuseColor;
	float3 Direction;
	float Intensity;
};

struct PointLight
{
	float4 Color;
	float3 Position;
	float Range;
	float Intensity;
	float3 Padding;
};

struct LightList
{
	uint PointLightIndices[MAX_GRID_POINT_LIGHT_NUM];
	uint NumPointLights;
	uint SpotLightIndices[MAX_GRID_SPOTLIGHT_NUM];
	uint NumSpotlights;
};

cbuffer externalData : register(b0)
{
	DirectionalLight dirLight[MAX_DIRECTIONAL_LIGHT_NUM];
	PointLight pointLight[MAX_POINT_LIGHT_NUM];
	float3 cameraPosition;
	int pointLightCount;
	int dirLightCount;
}

float Attenuate(float3 position, float range, float3 worldPos)
{
#if UNREAL_LIGHT_ATTENUATION
	float dist = distance(position, worldPos);
	float numer = dist / range;
	numer = numer * numer;
	numer = numer * numer;
	numer = saturate(1 - numer);
	numer = numer * numer;
	float denom = dist * dist + 1;
	return (numer / denom);
#else
	float dist = distance(position, worldPos);
	float att = saturate(1.0f - (dist * dist / (range * range)));
	return att * att;
#endif
}

// Lambert diffuse 
float3 LambertDiffuse(float3 kS, float3 albedo, float metalness)
{
	float3 kD = (float3(1.0f, 1.0f, 1.0f) - kS) * (1 - metalness);
	return (kD * albedo / PI);
}

// GGX (Trowbridge-Reitz)
float SpecDistribution(float3 n, float3 h, float roughness)
{
	float NdotH = max(dot(n, h), 0.0f);
	float NdotH2 = NdotH * NdotH;
	float a = roughness * roughness;
	float a2 = max(a * a, MIN_ROUGHNESS);

	// ((n dot h)^2 * (a^2 - 1) + 1)
	float denomToSquare = NdotH2 * (a2 - 1) + 1;

	return a2 / (PI * denomToSquare * denomToSquare);
}

float3 Fresnel_Schlick(float3 v, float3 h, float3 f0)
{
	float VdotH = max(dot(v, h), 0.0f);
	return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

float3 Fresnel_Epic(float3 v, float3 h, float3 f0)
{
	float VdotH = max(dot(v, h), 0.0f);
	return f0 + (1 - f0) * exp2((-5.55473 * VdotH - 6.98316) * VdotH);
}

// Fresnel term - Schlick approx.
float3 Fresnel(float3 v, float3 h, float3 f0)
{
	return Fresnel_Epic(v, h, f0);
}

float3 FresnelSchlickRoughness(float3 v, float3 n, float3 f0, float roughness)
{
	float NdotV = max(dot(v, n), 0.0f);
	float r1 = 1.0f - roughness;
	return f0 + (max(float3(r1, r1, r1), f0) - f0) * pow(1 - NdotV, 5.0f);
}

// Schlick-GGX 
float GeometricShadowing(float3 n, float3 v, float3 h, float roughness)
{
	// End result of remapping:
	float k = pow(roughness + 1, 2) / 8.0f;
	float NdotV = max(dot(n, v), 0.0f);

	// Final value
	return NdotV / (NdotV * (1 - k) + k);
}

// Cook-Torrance Specular
float3 CookTorrance(float3 n, float3 l, float3 v, float roughness, float metalness, float3 f0, out float3 kS)
{
	float3 h = normalize(v + l);

	float D = SpecDistribution(n, h, roughness);
	float3 F = Fresnel(v, h, f0);
	float G = GeometricShadowing(n, v, h, roughness) * GeometricShadowing(n, l, h, roughness);
	kS = F;
	//return (D * F * G) / (4 * max(dot(n, v), dot(n, l)));
	float NdotV = max(dot(n, v), 0.0f);
	float NdotL = max(dot(n, l), 0.0f);
	return (D * F * G) / (4 * max(NdotV * NdotL, 0.01f));
}

float3 AmbientPBR(float3 kD, float metalness, float3 diffuse, float ao, float3 specular)
{
	kD *= (1.0f - metalness);
	return (kD * diffuse + specular) * ao;
}

float3 AmbientPBR(float3 normal, float3 worldPos,
	float3 camPos, float roughness, float metalness,
	float3 albedo, float3 irradiance, float3 prefilteredColor, float2 brdf, float shadowAmount)
{
	float3 f0 = lerp(F0_NON_METAL.rrr, albedo.rgb, metalness);
	float ao = 1.0f;
	float3 toCam = normalize(camPos - worldPos);

	float3 kS = FresnelSchlickRoughness(toCam, normal, f0, roughness);
	float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
	kD *= (1.0f - metalness);

	float3 specular = prefilteredColor * (kS * brdf.x + float3(brdf.y, brdf.y, brdf.y));
	float3 diffuse = irradiance * albedo;

	float3 ambient = (kD * diffuse + specular) * ao;

	return ambient;
}

float3 DirectPBR(float lightIntensity, float3 lightColor, float3 toLight, float3 normal, float3 worldPos, float3 camPos, float roughness, float metalness, float3 albedo, float shadowAmount)
{
	float3 f0 = lerp(F0_NON_METAL.rrr, albedo.rgb, metalness);
	float ao = 1.0f;
	float3 toCam = normalize(camPos - worldPos);
	float3 kS = float3(0.f, 0.f, 0.f);
	float3 specBRDF = CookTorrance(normal, toLight, toCam, roughness, metalness, f0, kS);
	float3 diffBRDF = LambertDiffuse(kS, albedo, metalness);

	float NdotL = max(dot(normal, toLight), 0.0);

	return (diffBRDF + specBRDF) * NdotL * lightIntensity * lightColor.rgb * shadowAmount;
}

#endif
