
#include "StaticSamplers.hlsli"
#include "ShaderUtil.hlsli"
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"

#define GTAO_RADIUS 250.0f
#define GTAO_THICKNESS 1.0f

#define GTAO_CIRCLE_NUM 2
#define GTAO_SLICE_NUM 2
#define GTAO_POWER 2.5f
#define GTAO_INTENSITY 1.0f
#define GTRO_INTENSITY 0.4f

#define USE_COSINE_WEIGHT 1

#define PI 3.14159265359f
#define HALF_PI 1.570796326795

cbuffer constBuffer : register(b0)
{
	float _UvToViewScaleX;
	float _UvToViewScaleY;
	float _UvToViewOffsetX;
	float _UvToViewOffsetY;
	float _HalfProjectScale;
	float _TemporalOffsets;
	float _TemporalDirections;
};

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

// Input textures.
Texture2D gDepthTexture				: register(t0);
Texture2D gNormalTexture			: register(t1);
Texture2D gOrmTexture				: register(t2);

inline half3 GetPosition(half2 uv)
{
	half depth = gDepthTexture.SampleLevel(linearClampSampler, uv, 0).r;
	half viewDepth = ViewDepth(depth);
	return half3((float2(uv.x, 1.0f - uv.y) * half2(_UvToViewScaleX, _UvToViewScaleY) + half2(_UvToViewOffsetX, _UvToViewOffsetY)) * viewDepth, viewDepth);
}

inline half3 GetNormal(half2 uv)
{
	half3 Normal = gNormalTexture.Sample(linearClampSampler, uv).rgb;
	half3 view_Normal = normalize(mul(Normal, (half3x3)gView));

	return view_Normal;
}

float3 ReconstructWorldPos(float2 uv, float depth)
{
	float ndcX = uv.x * 2 - 1;
	float ndcY = 1 - uv.y * 2; // Remember to flip y!!!
	float4 viewPos = mul(float4(ndcX, ndcY, depth, 1.0f), gInvProj);
	viewPos = viewPos / viewPos.w;
	return mul(viewPos, gInvView).xyz;
}

inline float ConeConeIntersection(float ArcLength0, float ArcLength1, float AngleBetweenCones)
{
	float AngleDifference = abs(ArcLength0 - ArcLength1);
	float AngleBlendAlpha = saturate((AngleBetweenCones - AngleDifference) / (ArcLength0 + ArcLength1 - AngleDifference));
	return smoothstep(0, 1, 1 - AngleBlendAlpha);
}

inline half ReflectionOcclusion(half3 BentNormal, half3 ReflectionVector, half Roughness, half OcclusionStrength)
{
	half BentNormalLength = length(BentNormal);

	half ReflectionConeAngle = max(Roughness, 0.04) * PI;
	half UnoccludedAngle = BentNormalLength * PI * OcclusionStrength;
	half AngleBetween = acos(dot(BentNormal, ReflectionVector) / max(BentNormalLength, 0.001));

	half ReflectionOcclusion = ConeConeIntersection(ReflectionConeAngle, UnoccludedAngle, AngleBetween);
	return lerp(0, ReflectionOcclusion, saturate((UnoccludedAngle - 0.1) / 0.2));
}

inline half3 MultiBounce(half AO, half3 Albedo)
{
	half3 A = 2 * Albedo - 0.33;
	half3 B = -4.8 * Albedo + 0.64;
	half3 C = 2.75 * Albedo + 0.69;
	return max(AO, ((AO * A + B) * AO + C) * AO);
}

inline half GTAO_Offsets(half2 uv)
{
	int2 position = (int2)(uv * gRenderTargetSize * GTAO_RESOLUTION_SCALE);
	return 0.25 * (half)((position.y - position.x) & 3);
}

inline half GTAO_Noise(half2 position)
{
	return frac(52.9829189 * frac(dot(position, half2(0.06711056, 0.00583715))));
}

half IntegrateArc_UniformWeight(half2 h)
{
	half2 Arc = 1 - cos(h);
	return Arc.x + Arc.y;
}

half IntegrateArc_CosWeight(half2 h, half n)
{
	half2 Arc = -cos(2 * h - n) + cos(n) + 2 * h * sin(n);
	return 0.25 * (Arc.x + Arc.y);
}

half4 GTAO(half2 uv, int NumCircle, int NumSlice, inout half Depth)
{
	half3 vPos = GetPosition(uv);
	half3 viewNormal = GetNormal(uv);
	half3 viewDir = normalize(0 - vPos);

	half stepRadius = (max(min((GTAO_RADIUS * _HalfProjectScale) / vPos.b, 512), (half)NumSlice)) / ((half)NumSlice + 1);
	half noiseOffset = frac(GTAO_Offsets(half2(uv.x, 1.0f - uv.y)) + _TemporalOffsets);
	half noiseDirection = GTAO_Noise(half2(uv.x, 1.0f - uv.y) * gRenderTargetSize * GTAO_RESOLUTION_SCALE) + _TemporalDirections;
	half2 gtaoTexelSize = gInvRenderTargetSize / GTAO_RESOLUTION_SCALE;

	half Occlusion, angle, BentAngle, wallDarkeningCorrection, sliceLength, n, cos_n;
	half2 slideDir_TexelSize, h, H, falloff, uvOffset, h1h2, h1h2Length;
	half3 sliceDir, h1, h2, planeNormal, planeTangent, sliceNormal, BentNormal;
	half4 uvSlice;

	Occlusion = 0.0f;
	BentNormal = half3(0.0f, 0.0f, 0.0f);

	//if (gDepthTexture.Sample(linearClampSampler, uv).r <= 1e-7)
		//return 1;

	[unroll]
	for (int i = 0; i < NumCircle; i++)
	{
		angle = (i + noiseDirection) * (PI / (half)NumCircle);
		sliceDir = half3(half2(cos(angle), sin(angle)), 0);

		planeNormal = normalize(cross(sliceDir, viewDir));
		planeTangent = cross(viewDir, planeNormal);
		sliceNormal = viewNormal - planeNormal * dot(viewNormal, planeNormal);
		sliceLength = length(sliceNormal);

		cos_n = clamp(dot(normalize(sliceNormal), viewDir), -1, 1);
		n = -sign(dot(sliceNormal, planeTangent)) * acos(cos_n);
		h = half2(-1.0f, -1.0f);

		[unroll]
		for (int j = 0; j < NumSlice; j++)
		{
			uvOffset = (sliceDir.xy * gtaoTexelSize.xy) * max(stepRadius * (j + noiseOffset), 1 + j) * half2(1.0f, -1.0f);
			uvSlice = uv.xyxy + float4(uvOffset.xy, -uvOffset.xy);

			h1 = GetPosition(uvSlice.xy) - vPos;
			h2 = GetPosition(uvSlice.zw) - vPos;

			h1h2 = half2(dot(h1, h1), dot(h2, h2));
			h1h2Length = rsqrt(h1h2);

			falloff = saturate(h1h2 * (2 / max(GTAO_RADIUS * GTAO_RADIUS, 0.001f)));

			H = half2(dot(h1, viewDir), dot(h2, viewDir)) * h1h2Length;
			h.xy = (H.xy > h.xy) ? lerp(H, h, falloff) : lerp(H.xy, h.xy, GTAO_THICKNESS);
		}	

		h = acos(clamp(h, -1, 1));
		h.x = n + max(-h.x - n, -HALF_PI);
		h.y = n + min(h.y - n, HALF_PI);

		BentAngle = (h.x + h.y) * 0.5;
		BentNormal += (viewDir * cos(BentAngle) - planeTangent * sin(BentAngle));

#if USE_COSINE_WEIGHT
		Occlusion += sliceLength * IntegrateArc_CosWeight(h, n);
#else
		Occlusion += sliceLength * IntegrateArc_UniformWeight(h);
#endif

	}

	BentNormal = normalize(normalize(BentNormal) - viewDir * 0.5);
	Occlusion = saturate(pow(Occlusion / (half)NumCircle, GTAO_POWER));
	Depth = vPos.b;

	return half4(BentNormal, Occlusion);
}

half3 main(VertexToPixel i) : SV_Target
{
	half2 uv = i.uv;

	half SSAODepth = 0;
	half4 GT_Details = GTAO(uv, GTAO_CIRCLE_NUM, GTAO_SLICE_NUM, SSAODepth);

	half3 BentNormal = mul(GT_Details.rgb, (half3x3)gInvView);
	half3 WorldNormal = gNormalTexture.Sample(linearClampSampler, uv).rgb;
	half Roughness = gOrmTexture.Sample(basicSampler, uv).g;
	half SceneDepth = gDepthTexture.Sample(linearClampSampler, uv).r;
	half3 WorldPos = ReconstructWorldPos(uv, SceneDepth);
	half3 ViewDir = normalize(WorldPos - gEyePosW);
	half3 ReflectionDir = reflect(ViewDir, WorldNormal);

	//half GTRO = ReflectionOcclusion_LUT(uv, GT_Details.a, Roughness, BentNormal);
	half GTRO = ReflectionOcclusion(BentNormal, ReflectionDir, Roughness, lerp(1.0f, GT_Details.a, GTRO_INTENSITY));
	GTRO = GTRO * GTRO;
	half GTAO = lerp(1.0f, GT_Details.a, GTAO_INTENSITY);

	return half3(GTAO, GTRO, SSAODepth);
}

