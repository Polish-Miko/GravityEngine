
#include "StaticSamplers.hlsli"
#include "ShaderUtil.hlsli"
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"


#define NUM_STEPS 60
#define BRDF_BIAS 0.0f

#define HIZ_START_LEVEL 0
#define HIZ_STOP_LEVEL 0

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

cbuffer constBuffer : register(b0)
{
	float _UvToViewScaleX;
	float _UvToViewScaleY;
	float _UvToViewOffsetX;
	float _UvToViewOffsetY;
};

// Input textures.
Texture2D gDepthTexture							: register(t0);
Texture2D gNormalTexture						: register(t1);
Texture2D gOrmTexture							: register(t2);
Texture2D gColorTexture							: register(t3);
Texture2D gBlueNoiseTexture						: register(t4);

Texture2D gHiZTexture[SSR_MAX_MIP_LEVEL]		: register(t0, space1);

float3 GetScreenPos(float2 uv, float depth)
{
	//return float3(uv.xy * 2 - 1, depth);
	return float3(uv.x * 2.0f - 1.0f, 1.0f - 2.0f * uv.y, depth);
}

float3 ReconstructWorldPos(float2 uv, float depth)
{
	float ndcX = uv.x * 2 - 1;
	float ndcY = 1 - uv.y * 2; // Remember to flip y!!!
	float4 viewPos = mul(float4(ndcX, ndcY, depth, 1.0f), gUnjitteredInvProj);
	viewPos = viewPos / viewPos.w;
	return mul(viewPos, gInvView).xyz;
}

float3 GetViewDir(float3 worldPos)
{
	return normalize(worldPos - gEyePosW);
}

float3 GetViewPos(float3 screenPos)
{
	float4 viewPos = mul(float4(screenPos, 1), gUnjitteredInvProj);
	return viewPos.xyz / viewPos.w;
}

inline float3 GetViewPosNew(half2 uv, half2 jitteredUV)
{
	half depth = gDepthTexture.SampleLevel(linearClampSampler, jitteredUV, 0).r;
	half viewDepth = ViewDepth(depth);
	return half3((float2(uv.x, 1.0f - uv.y) * half2(_UvToViewScaleX, _UvToViewScaleY) + half2(_UvToViewOffsetX, _UvToViewOffsetY)) * viewDepth, viewDepth);
}

float4 TangentToWorld(float3 N, float4 H)
{
	float3 UpVector = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 T = normalize(cross(UpVector, N));
	float3 B = cross(N, T);

	return float4((T * H.x) + (B * H.y) + (N * H.z), H.w);
}

// Brian Karis, Epic Games "Real Shading in Unreal Engine 4"
float4 ImportanceSampleGGX(float2 Xi, float Roughness)
{
	float m = Roughness * Roughness;
	float m2 = m * m;

	float Phi = 2 * _PI * Xi.x;

	float CosTheta = sqrt((1.0 - Xi.y) / (1.0 + (m2 - 1.0) * Xi.y));
	float SinTheta = sqrt(max(1e-5, 1.0 - CosTheta * CosTheta));

	float3 H;
	H.x = SinTheta * cos(Phi);
	H.y = SinTheta * sin(Phi);
	H.z = CosTheta;

	float d = (CosTheta * m2 - CosTheta) * CosTheta + 1;
	float D = m2 / (_PI * d * d);
	float pdf = D * CosTheta;

	return float4(H, pdf);
}

float4 RayMarch(Texture2D depthTex, float4x4 projectionMatrix, float3 viewDir, int NumSteps, float3 viewPos, float3 screenPos, float2 screenUV, float stepSize, float thickness)
{
	//float3 dirProject = _Project;
	/*
	float4 dirProject = float4
		(
			abs(unity_CameraProjection._m00 * 0.5),
			abs(unity_CameraProjection._m11 * 0.5),
			((_ProjectionParams.z * _ProjectionParams.y) / (_ProjectionParams.y - _ProjectionParams.z)) * 0.5,
			0.0
			);
	*/

	//float linearDepth = ViewDepth(depthTex.SampleLevel(linearClampSampler, screenUV.xy, 0.0f).r);

	// transform ray direction from view space to ndc.
	float4 rayProj = mul(float4(viewDir + viewPos, 1.0f), projectionMatrix);
	rayProj /= rayProj.w;
	rayProj.y = -rayProj.y;
	float2 rayDirXY = (rayProj.xy - (screenUV * 2.0f - 1.0f)); // ndc to uv(*0.5f)
	float lenXY = length(rayDirXY);
	float rayDirZ = ViewDepth(rayProj.z) - ViewDepth(screenPos.z);
	float3 rayDir = float3(rayDirXY, rayDirZ) / lenXY;

	/*
	float3 ray = viewPos / viewPos.z;
	float3 rayDir = normalize(float3(viewDir.xy - ray * viewDir.z, viewDir.z / linearDepth) * dirProject);
	rayDir.xy *= 0.5;
	*/

	float3 rayStart = float3(screenUV, ViewDepth(screenPos.z));

	float3 samplePos = rayStart + rayDir * stepSize;

	//float project = (_ProjectionParams.z * _ProjectionParams.y) / (_ProjectionParams.y - _ProjectionParams.z);

	//float thickness = thickness;//1.0 / _ZBufferParams.x * (float)NumSteps * linearDepth;//project / linearDepth;

	float mask = 0.0;

	float oldDepth = samplePos.z;
	float oldDelta = 0.0;
	float3 oldSamplePos = samplePos;

	[loop]
	for (int i = 0; i < NumSteps; i++)
	{
		float depth = ViewDepth(depthTex.SampleLevel(linearClampSampler, samplePos.xy + gJitter, 0.0f).r);
		float delta = samplePos.z - depth;
		//float thickness = dirProject.z / depth;

		if (0.0 < delta)
		{
			if (delta /*< thickness*/)
			{
				mask = 1.0;
				break;
				//samplePos = samplePos;
			}
			/*if(depth - oldDepth > thickness)
			{
				float blend = (oldDelta - delta) / max(oldDelta, delta) * 0.5 + 0.5;
				samplePos = lerp(oldSamplePos, samplePos, blend);
				mask = lerp(0.0, 1.0, blend);
			}*/
		}
		else
		{
			oldDelta = -delta;
			oldSamplePos = samplePos;
		}
		oldDepth = depth;
		samplePos += rayDir * stepSize;
	}

	return float4(samplePos, mask);
}

float GetMarchSize(float2 dir, float2 pixelSize)
{
	return length(float2(min(abs(dir.x), pixelSize.x), min(abs(dir.y), pixelSize.y)));
}

half4 Hierarchical_Z_Trace
(
	int HiZ_Max_Level,
	int NumSteps,
	float thickness,
	float2 RayCastTexelSize,
	float3 rayStart,
	float3 rayDir
)
{
	half SamplerSize = GetMarchSize(rayDir.xy, RayCastTexelSize);
	half3 samplePos = rayStart + rayDir * SamplerSize * 5.0f;
	int level = HIZ_START_LEVEL;
	half mask = 0.0;

	[loop]
	for (int i = 0; i < NumSteps; i++)
	{
		float2 cellCount = RayCastTexelSize * exp2(level);
		float newSamplerSize = GetMarchSize(rayDir.xy, cellCount);
		half3 newSamplePos = samplePos + rayDir * newSamplerSize;
		float sampleMinDepth = gHiZTexture[max(level, 0)].SampleLevel(linearClampSampler, newSamplePos.xy + gJitter, 0.0f).r;// ViewDepth(gHiZTexture[max(level, 0)].SampleLevel(linearClampSampler, newSamplePos.xy + gJitter, 0.0f).r);//
	
		[flatten]
		if (sampleMinDepth < newSamplePos.z)
		{
			level = min(HiZ_Max_Level - 1, level + 1.0);
			samplePos = newSamplePos;
		}
		else
		{
			level--;
		}

		[branch]
		if (samplePos.x < 0.0f ||
			samplePos.x > 1.0f ||
			samplePos.y < 0.0f ||
			samplePos.y > 1.0f
			)
		{
			return half4(samplePos, mask);
		}

		[branch]
		if (level < HIZ_STOP_LEVEL)
		{
			float delta = samplePos.z - sampleMinDepth;
			mask = i > 0.0/* && delta <= thickness*/;
			return half4(samplePos, mask);
		}
	}
	return half4(samplePos, mask);
}

// Velocity texture setup
half4 main(VertexToPixel i) : SV_Target
{
	float2 uv = i.uv;
	float2 jitteredUV = uv + gJitter;
	//int2 pos = uv /* _RayCastSize.xy*/;

	float3 worldNormal = gNormalTexture.SampleLevel(linearClampSampler, jitteredUV, 0.0f).rgb;
	float3 viewNormal = normalize(mul(worldNormal, (half3x3)gView));
	float roughness = gOrmTexture.SampleLevel(linearClampSampler, jitteredUV, 0.0f).g;

	float depth = gDepthTexture.SampleLevel(linearClampSampler, jitteredUV, 0.0f).r;
	float3 screenPos = GetScreenPos(uv, depth);

	float3 worldPos = ReconstructWorldPos(uv, depth);
	float3 viewDir = GetViewDir(worldPos);

	float3 viewPos = GetViewPos(screenPos);//GetViewPosNew(uv, jitteredUV);

	// Blue noise generated by https://github.com/bartwronski/BlueNoiseGenerator/;
	float2 jitter = gBlueNoiseTexture.SampleLevel(linearWrapSampler, (uv + gHaltonUniform2D.xy) * (gRenderTargetSize.xy * 0.5f) / float2(BLUE_NOISE_SIZE, BLUE_NOISE_SIZE), 0.0f).rg;
	
	float2 Xi = jitter;

	Xi.y = lerp(Xi.y, 0.0, BRDF_BIAS);

	float4 H = TangentToWorld(worldNormal, ImportanceSampleGGX(Xi, roughness));
	float3 dir = reflect(viewDir, H.xyz);
	//dir = reflect(viewDir, worldNormal);
	dir = normalize(mul(dir, (float3x3)gView));
	//dir = normalize(reflect(viewPos, viewNormal));
	//return float4(dir, 1.0f);

	/*
	float4 rayProj = mul(float4(dir + viewPos, 1.0f), gProj);
	rayProj /= rayProj.w;
	rayProj.y = -rayProj.y;
	float2 rayDirXY = normalize(rayProj.xy - (uv * 2.0f - 1.0f)) * 0.5f; // ndc to uv(*0.5f)
	float rayDirZ = ViewDepth(rayProj.z) - ViewDepth(screenPos.z);
	float3 rayDir = float3(rayDirXY, rayDirZ);
	return float4(rayDirZ, rayDirZ, rayDirZ, 1.0f);
	*/

	/*
	jitter += 0.5f;

	float stepSize = (1.0 / (float)NUM_STEPS);
	stepSize = stepSize * (jitter.x + jitter.y) + stepSize;

	float2 rayTraceHit = 0.0;
	float rayTraceZ = 0.0;
	float rayPDF = 0.0;
	float rayMask = 0.0;
	float4 rayTrace = RayMarch(gDepthTexture, gProj, dir, NUM_STEPS, viewPos, screenPos, uv, stepSize, 1.0);

	rayTraceHit = rayTrace.xy;
	rayTraceZ = rayTrace.z;
	rayPDF = H.w;
	rayMask = rayTrace.w;

	float4 outRayCast = float4(float3(rayTraceHit, rayTraceZ), rayPDF);
	float outRayCastMask = rayMask;
	*/

	float3 ReflectionDir = reflect(normalize(viewPos), viewNormal);
	float3 rayStart = float3(uv, depth);
	float4 rayProj = mul(float4(viewPos + ReflectionDir, 1.0), gUnjitteredProj);
	float3 rayDir = normalize((rayProj.xyz / rayProj.w) - screenPos);
	rayDir.xy *= float2(0.5f, -0.5f);
	//rayDir.z = ReflectionDir.z;
	
	/*
	float3 rayStart = float3(uv, ViewDepth(gHiZTexture[0].SampleLevel(linearClampSampler, jitteredUV, 0.0f).r));//float3(uv, ViewDepth(screenPos.z));//
	float4 rayProj = mul(float4(dir + viewPos, 1.0f), gUnjitteredProj);
	rayProj /= rayProj.w;
	rayProj.y = -rayProj.y;
	float2 rayDirXY = (rayProj.xy - (uv * 2.0f - 1.0f)); // ndc to uv(*0.5f)
	float lenXY = length(rayDirXY);
	float rayDirZ = ViewDepth(rayProj.z) - ViewDepth(screenPos.z);
	float3 rayDir = float3(rayDirXY, rayDirZ) / lenXY;
	//return float4(rayDir.xy, 0.0f, 1.0f);
	*/

	float4 rayTrace = Hierarchical_Z_Trace(SSR_MAX_MIP_LEVEL, NUM_STEPS, 1000.0f, gInvRenderTargetSize.xy * 2.0f, rayStart, rayDir);
	float4 outRayCast = rayTrace;
	float rayMask = rayTrace.w;

	float3 outColor = float3(1.0f, 0.0f, 0.0f);
	if (rayMask > 0.5f)
		outColor = gColorTexture.SampleLevel(linearClampSampler, outRayCast.xy + gJitter, 0.0f).rgb;
	//outColor = float3(rayMask, rayMask, rayMask);
	//outColor = float3(jitter, 0.0f);

	return float4(outColor, 1.0f);
}

