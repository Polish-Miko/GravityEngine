
#ifndef CLUSTERED_DEFERRED_CS_HLSL
#define CLUSTERED_DEFERRED_CS_HLSL


#include "ShaderDefinition.h"
#include "MainPassCB.hlsli"

#define READBACK_REVERSED_Z

Texture2D gDepthBuffer : register(t0);

RWStructuredBuffer<float> gDepthDownsampleBuffer : register(u0);

static const float2 gDepthReadbackBufferSize = float2(DEPTH_READBACK_BUFFER_SIZE_X, DEPTH_READBACK_BUFFER_SIZE_Y);

SamplerState gSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

[numthreads(DEPTH_DOWNSAMPLE_THREAD_NUM_X, DEPTH_DOWNSAMPLE_THREAD_NUM_Y, 1)]
void main(
	uint3 groupId          : SV_GroupID,
	uint3 dispatchThreadId : SV_DispatchThreadID,
	uint3 groupThreadId : SV_GroupThreadID
	)
{
    uint groupIndex = dispatchThreadId.y * DEPTH_READBACK_BUFFER_SIZE_X + dispatchThreadId.x;

    uint2 globalCoords = dispatchThreadId.xy;
	float2 uv = globalCoords / gDepthReadbackBufferSize;

	float2 offset = (gRenderTargetSize / gDepthReadbackBufferSize) / 3;
	float2 uvOffset = offset / gDepthReadbackBufferSize;

	float depthFromTexture;
	float2 sampUV;
	float maxDepth;

#if USE_REVERSE_Z
#ifdef READBACK_REVERSED_Z
	maxDepth = 1.0f;
#else
	maxDepth = 0.0f;
#endif
#else
	maxDepth = 0.0f;
#endif

	for (int x = -1; x < 2; x++)
	{
		for (int y = -1; y < 2; y++)
		{
			sampUV = saturate(uv + float2(x, y) * uvOffset);

			depthFromTexture = gDepthBuffer.SampleLevel(gSampler, sampUV, 0).r;

#if USE_REVERSE_Z
#ifndef READBACK_REVERSED_Z
			depthFromTexture = 1 - depthFromTexture;
#endif
#endif

#if USE_REVERSE_Z
#ifdef READBACK_REVERSED_Z
			if (depthFromTexture < maxDepth)
#else
			if (depthFromTexture > maxDepth)
#endif
#else
			if (depthFromTexture > maxDepth)
#endif
			{ 
				maxDepth = depthFromTexture;
			}
		}
	}

	gDepthDownsampleBuffer[groupIndex] = maxDepth;

}


#endif



