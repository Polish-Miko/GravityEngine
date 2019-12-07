
#include "StaticSamplers.hlsli"
#include "ShaderUtil.hlsli"
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"

#define DELTA_DEPTH_UPPER_BOUND 100.0f

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

// Input textures.
Texture2D gOcclusionTexture				: register(t0);
Texture2D gDepthTexture					: register(t1);
Texture2D gVelocityTexture				: register(t2);

half2 main(VertexToPixel i) : SV_Target
{
	// Bilateral upsample.
	half2 UV = i.uv;
	half2 SamplerSize = gRenderTargetSize.xy;

	half SceneDepth = gDepthTexture.Sample(linearClampSampler, UV).r;
	half EyeDepth = ViewDepth(SceneDepth);

	float4 TopLeft_Color = gOcclusionTexture.Sample(linearClampSampler, UV);
	float4 TopRight_Color = gOcclusionTexture.Sample(linearClampSampler, UV + (float2(0.0, 1.0) / SamplerSize));
	float4 BottomLeft_Color = gOcclusionTexture.Sample(linearClampSampler, UV + (float2(1.0, 0.0) / SamplerSize));
	float4 BottomRight_Color = gOcclusionTexture.Sample(linearClampSampler, UV + (float2(1.0, 1.0) / SamplerSize));

	float TopLeft_Depth = EyeDepth;
	float TopRight_Depth = ViewDepth(gDepthTexture.Sample(linearClampSampler, UV + (float2(0.0, 1.0) / SamplerSize)).r);
	float BottomLeft_Depth = ViewDepth(gDepthTexture.Sample(linearClampSampler, UV + (float2(1.0, 0.0) / SamplerSize)).r);
	float BottomRight_Depth = ViewDepth(gDepthTexture.Sample(linearClampSampler, UV + (float2(1.0, 1.0) / SamplerSize)).r);

	float4 offsetDepths = float4(TopLeft_Depth, TopRight_Depth, BottomLeft_Depth, BottomRight_Depth);
	float4 weights = saturate(1.0 - abs(offsetDepths - EyeDepth) / DELTA_DEPTH_UPPER_BOUND);

	float2 fractCoord = frac(UV * SamplerSize);

	float4 filteredX0 = lerp(TopLeft_Color * weights.x, TopRight_Color * weights.y, fractCoord.x);
	float4 filteredX1 = lerp(BottomRight_Color * weights.w, BottomLeft_Color * weights.z, fractCoord.x);
	float4 filtered = lerp(filteredX0, filteredX1, fractCoord.y);

	return float2(filtered.rg);
}

