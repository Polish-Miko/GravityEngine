
#include "StaticSamplers.hlsli"
#include "ShaderUtil.hlsli"
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"


#define NUM_STEPS 60
#define BRDF_BIAS 0.7f

#define HIZ_START_LEVEL 0
#define HIZ_STOP_LEVEL 0

#define THICKNESS 50.0f

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

struct PixelOutput
{
	half4	RayHit						: SV_TARGET0;
	half	Mask						: SV_TARGET1;
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

float3 intersectDepth_Plane(float3 rayOrigin, float3 rayDir, float marchSize)
{
	return rayOrigin + rayDir * marchSize;
}

float2 cell(float2 ray, float2 cell_count) 
{
	return floor(ray.xy * cell_count);
}

float2 cell_count(float level, float2 ScreenSize)
{
	return ScreenSize / (level == 0 ? 1 : exp2(level));
}

float3 intersect_cell_boundary(float3 rayOrigin, float3 rayDir, float2 cellIndex, float2 cellCount, float2 crossStep, float2 crossOffset)
{
	float2 cell_size = 1.0 / cellCount;
	float2 planes = cellIndex / cellCount + cell_size * crossStep;

	float2 solutions = (planes - rayOrigin.xy) / rayDir.xy;
	float3 intersection_pos = rayOrigin + rayDir * min(solutions.x, solutions.y);

	intersection_pos.xy += (solutions.x < solutions.y) ? float2(crossOffset.x, 0.0) : float2(0.0, crossOffset.y);

	return intersection_pos;
}

bool crossed_cell_boundary(float2 cell_id_one, float2 cell_id_two)
{
	return (int)cell_id_one.x != (int)cell_id_two.x || (int)cell_id_one.y != (int)cell_id_two.y;
}

float minimum_depth_plane(float2 ray, float level, float2 cell_count)
{
	return gHiZTexture[max(level, 0)].Load(int3((ray * cell_count), 0)).r;
}

float4 Hierarchical_Z_Trace(int HiZ_Max_Level, int HiZ_Start_Level, int HiZ_Stop_Level, int NumSteps, float Thickness, float2 screenSize, float3 rayOrigin, float3 rayDir)
{
	HiZ_Max_Level = clamp(HiZ_Max_Level, 0.0, 7.0);
	rayOrigin = half3(rayOrigin.x, rayOrigin.y, rayOrigin.z);
	rayDir = half3(rayDir.x, rayDir.y, rayDir.z);

	float level = HiZ_Start_Level;
	float3 ray = rayOrigin + rayDir * 0.05f;

	float2 cross_step = float2(rayDir.x >= 0.0 ? 1.0 : -1.0, rayDir.y >= 0.0 ? 1.0 : -1.0);
	float2 cross_offset = cross_step * 0.00001f;
	cross_step = saturate(cross_step);

	float2 hi_z_size = cell_count(level, screenSize);
	float2 ray_cell = cell(ray.xy, hi_z_size.xy);
	ray = intersect_cell_boundary(ray, rayDir, ray_cell, hi_z_size, cross_step, cross_offset);

	int iterations = 0;
	float mask = 1.0f;
	while (level >= HiZ_Stop_Level && iterations < NumSteps) 
	{
		float3 tmp_ray = ray;
		float2 current_cell_count = cell_count(level, screenSize);
		float2 old_cell_id = cell(ray.xy, current_cell_count);

		if (ray.x < 0.0f ||
			ray.x > 1.0f ||
			ray.y < 0.0f ||
			ray.y > 1.0f
			)
		{
			mask = 0.0f;
			return half4(ray.xy, ray.z, mask);
		}

		float min_z = minimum_depth_plane(ray.xy, level, current_cell_count);

		if (min_z < 1e-7)
		{
			mask = 0.0f;
			return half4(ray.xy, ray.z, mask);
		}

		if (rayDir.z < 0)
		{
			float min_minus_ray = min_z - ray.z;
			tmp_ray = min_minus_ray < 0 ? ray + (rayDir / rayDir.z) * min_minus_ray : tmp_ray;
			float2 new_cell_id = cell(tmp_ray.xy, current_cell_count);

			if (crossed_cell_boundary(old_cell_id, new_cell_id))
			{
				tmp_ray = intersect_cell_boundary(ray, rayDir, old_cell_id, current_cell_count, cross_step, cross_offset);
				level = min(HiZ_Max_Level, level + 2.0);
			}
			/* 
			else 
			{
				if(level == 1.0 && abs(min_minus_ray) > 0.0001) 
				{
					tmp_ray = intersect_cell_boundary(ray, rayDir, old_cell_id, current_cell_count, cross_step, cross_offset);
					level = 2.0;
				}
			}
			*/
		}
		else if (ray.z > min_z)
		{
			tmp_ray = intersect_cell_boundary(ray, rayDir, old_cell_id, current_cell_count, cross_step, cross_offset);
			level = min(HiZ_Max_Level, level + 2.0);
		}

		ray.xyz = tmp_ray.xyz;
		level--;
		iterations++;

		mask = (ViewDepth(ray.z) - ViewDepth(min_z)) < Thickness && iterations > 0.0;
	}

	return half4(ray.xy, ray.z, mask);
}

PixelOutput main(VertexToPixel i)
{
	float2 uv = i.uv;
	float2 jitteredUV = uv + gJitter;

	float depth = gDepthTexture.SampleLevel(linearClampSampler, jitteredUV, 0.0f).r;
	float roughness = gOrmTexture.SampleLevel(linearClampSampler, jitteredUV, 0.0f).g;
	float3 worldNormal = gNormalTexture.SampleLevel(linearClampSampler, jitteredUV, 0.0f).rgb;
	float3 viewNormal = normalize(mul(worldNormal, (half3x3)gView));

	float3 screenPos = GetScreenPos(uv, depth);
	float3 worldPos = ReconstructWorldPos(uv, depth);
	float3 viewPos = GetViewPos(screenPos);
	float3 viewDir = GetViewDir(worldPos);

	half2 Hash = gBlueNoiseTexture.SampleLevel(linearWrapSampler, (uv + gHaltonUniform2D.xy) * (gRenderTargetSize.xy * 0.5f) / float2(BLUE_NOISE_SIZE, BLUE_NOISE_SIZE), 0.0f).rg;
	Hash.y = lerp(Hash.y, 0.0, BRDF_BIAS);

	half4 H = 0.0;
	if (roughness > 0.1f)
	{
		H = TangentToWorld(viewNormal, ImportanceSampleGGX(Hash, roughness));
	}
	else
	{
		H = half4(viewNormal, 1.0);
	}
	float3 ReflectionDir = reflect(normalize(viewPos), H.xyz);

	//float3 ReflectionDir = reflect(normalize(viewPos), viewNormal);
	float3 rayStart = float3(uv, depth);
	float4 rayProj = mul(float4(viewPos + ReflectionDir, 1.0), gUnjitteredProj);
	float3 rayDir = normalize((rayProj.xyz / rayProj.w) - screenPos);
	rayDir.xy *= float2(0.5f, -0.5f);

	//float4 rayTrace = Hierarchical_Z_Trace(SSR_MAX_MIP_LEVEL, NUM_STEPS, 1000.0f, gInvRenderTargetSize.xy * 2.0f, rayStart, rayDir);
	float4 rayTrace = Hierarchical_Z_Trace(SSR_MAX_MIP_LEVEL, 0, 0, NUM_STEPS, THICKNESS, gRenderTargetSize / 2.0f, rayStart, rayDir);
	float4 outRayCast = rayTrace;
	float rayMask = rayTrace.w;

	PixelOutput o;
	o.RayHit = half4(rayTrace.xyz, H.a);
	o.Mask = rayTrace.a;
	return o;

	/*
	float3 outColor = float3(1.0f, 0.0f, 0.0f);
	if (rayMask > 0.5f)
		outColor = gColorTexture.SampleLevel(linearClampSampler, outRayCast.xy + gJitter, 0.0f).rgb;
	//outColor = float3(rayMask, rayMask, rayMask);
	//outColor = float3(jitter, 0.0f);

	return float4(outColor, 1.0f);
	*/
}

