
#include "StaticSamplers.hlsli"
#include "ShaderUtil.hlsli"
#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

// Input textures.
Texture2D gVelocityBuffer				: register(t0);
Texture2D gDepthBuffer					: register(t1);

// Velocity texture setup
float4 main(VertexToPixel i) : SV_Target
{
	// Sample the motion vector.
	float2 v = gVelocityBuffer.Sample(basicSampler, i.uv).rg;

	// Apply the exposure time and convert to the pixel space.
	v *= (MOTION_BLUR_VELOCITY_SCALE * 0.5) * gRenderTargetSize.xy;

	// Clamp the vector with the maximum blur radius.
	v /= max(1.0, length(v) * MOTION_BLUR_RCP_MAX_RADIUS);

	// Sample the depth of the pixel.
	float d = Linear01Depth(gDepthBuffer.Sample(basicSampler, i.uv).r);

	// Pack into 10/10/10/2 format.
	return float4((v * MOTION_BLUR_RCP_MAX_RADIUS + 1.0) * 0.5, d, 0.0);
}

