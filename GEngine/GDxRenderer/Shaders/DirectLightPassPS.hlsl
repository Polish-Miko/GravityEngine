//#define POINT_INTENSITY 0.5
#include "Lighting.hlsli"

/*
cbuffer PerFrame : register(b1)
{
	float	nearZ;
	float	farZ;
	float2	lightPerspectiveValues;
};
*/

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

//G-Buffer
Texture2D gAlbedoTexture			: register(t0);
Texture2D gNormalTexture			: register(t1);
Texture2D gWorldPosTexture			: register(t2);
Texture2D gOrmTexture				: register(t3);
//Texture2D gMetalnessTexture			: register(t4); //t5, t6 for light pass

//IBL Textures
//Texture2D gDepth					: register(t7);
//TextureCube skyIrradianceTexture	: register(t8);
//Texture2D brdfLUTTexture			: register(t9);

//Shadow 
//Texture2D gShadowMap				: register(t11);
//Texture2D gShadowPos				: register(t12);
//TextureCube<float> gPointShadowMap	: register(t13);

SamplerState			basicSampler	: register(s0);
SamplerComparisonState	shadowSampler	: register(s1);

/*
float PointShadow(float3 ToPixel) 
{ 
	float3 ToPixelAbs = abs(ToPixel);
	float Z = max(ToPixelAbs.x, max(ToPixelAbs.y, ToPixelAbs.z));
	float Depth = (lightPerspectiveValues.x * Z + lightPerspectiveValues.y) / Z;
	return gPointShadowMap.SampleCmpLevelZero(shadowSampler, ToPixel, Depth);
}
*/


float4 main(VertexToPixel pIn) : SV_TARGET
{
	/*
	int3 sampleIndices = int3(pIn.position.xy, 0);
	float3 albedo = gAlbedoTexture.Load(sampleIndices).rgb;
	float3 normal = gNormalTexture.Load(sampleIndices).rgb;
	float3 worldPos = gWorldPosTexture.Load(sampleIndices).rgb;
	float roughness = gRoughnessTexture.Load(sampleIndices).r;
	float metal  = gMetalnessTexture.Load(sampleIndices).r;
	*/
	float4 packedAlbedo = gAlbedoTexture.Sample(basicSampler, pIn.uv);
	float3 albedo = packedAlbedo.rgb;
	float3 normal = gNormalTexture.Sample(basicSampler, pIn.uv).rgb;
	float3 worldPos = gWorldPosTexture.Sample(basicSampler, pIn.uv).rgb;
	float roughness = gOrmTexture.Sample(basicSampler, pIn.uv).g;
	float metal = gOrmTexture.Sample(basicSampler, pIn.uv).b;
	//float3 irradiance = skyIrradianceTexture.Sample(basicSampler, normal).rgb;
	//float shadowAmount = 1.f;
	/*
	if (pointLightIndex == 0)
	{
		shadowAmount = PointShadow(worldPos - pointLight[pointLightIndex].Position);
	}
	*/


	float3 finalColor = 0.f;
	float shadowAmount = 1.f;

	for (int i = 0; i < pointLightCount; i++)
	{
		shadowAmount = 1.f;
		float atten = Attenuate(pointLight[i].Position, pointLight[i].Range, worldPos);
		float lightIntensity = pointLight[i].Intensity * atten;
		float3 toLight = normalize(pointLight[i].Position - worldPos);
		float3 lightColor = pointLight[i].Color.rgb;

		//finalColor += PointLightPBR(pointLight[i], normalize(normal), worldPos, cameraPosition, roughness, metal, albedo, shadowAmount);
		finalColor = finalColor + DirectPBR(lightIntensity, lightColor, toLight, normalize(normal), worldPos, cameraPosition, roughness, metal, albedo, shadowAmount);
	}
	//float3 finalColor = PointLightPBR(pointLight[pointLightIndex], normalize(normal), worldPos, cameraPosition, roughness, metal, albedo, specColor);
	
	for (int i = 0; i < dirLightCount; i++)
	{
		float shadowAmount = 1.f;
		float lightIntensity = dirLight[i].Intensity;
		float3 toLight = normalize(-dirLight[i].Direction);
		float3 lightColor = dirLight[i].DiffuseColor.rgb;

		finalColor = finalColor + DirectPBR(lightIntensity, lightColor, toLight, normalize(normal), worldPos, cameraPosition, roughness, metal, albedo, shadowAmount);
		//finalColor = finalColor + DirLightPBR(dirLight[i], normalize(normal), worldPos, cameraPosition, roughness, metal, albedo, shadowAmount);
	}

	//return float4(packedAlbedo.rgb , 1.0f);
	return float4(finalColor, 1.0f);
}