//#define POINT_INTENSITY 0.5
#include "Lighting.hlsli"


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

SamplerState			basicSampler	: register(s0);
SamplerComparisonState	shadowSampler	: register(s1);



float4 main(VertexToPixel pIn) : SV_TARGET
{
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

		finalColor = finalColor + DirectPBR(lightIntensity, lightColor, toLight, normalize(normal), worldPos, cameraPosition, roughness, metal, albedo, shadowAmount);
	}
	
	for (int i = 0; i < dirLightCount; i++)
	{
		float shadowAmount = 1.f;
		float lightIntensity = dirLight[i].Intensity;
		float3 toLight = normalize(-dirLight[i].Direction);
		float3 lightColor = dirLight[i].DiffuseColor.rgb;

		finalColor = finalColor + DirectPBR(lightIntensity, lightColor, toLight, normalize(normal), worldPos, cameraPosition, roughness, metal, albedo, shadowAmount);
	}

	return float4(finalColor, 1.0f);
}