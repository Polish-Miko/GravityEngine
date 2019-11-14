
#ifndef SHADOW_MAP_PREFILTER_CS_HLSL
#define SHADOW_MAP_PREFILTER_CS_HLSL


#include "ShaderDefinition.h"

#define BLUR_RADIUS 3

static const float weights[4] = {
	0.006f,
	0.061f,
	0.242f,
	0.383f
};

cbuffer cbShadowMapPrefilter : register(b0)
{
	uint bHorizontally;
	//uint gPad0;
	//uint gPad1;
	//uint gPad2;
};

Texture2D gInput            : register(t0);
RWTexture2D<float> gOutput	: register(u0);

#define N 256
#define CacheSize (N + 2 * BLUR_RADIUS)
groupshared float gCache[CacheSize];

[numthreads(N, 1, 1)]
void main(
	int3 groupThreadID : SV_GroupThreadID,
	int3 dispatchThreadID : SV_DispatchThreadID
)
{
	if (bHorizontally)
	{
		//
		// Fill local thread storage to reduce bandwidth.  To blur 
		// N pixels, we will need to load N + 2*BlurRadius pixels
		// due to the blur radius.
		//

		// This thread group runs N threads.  To get the extra 2*BlurRadius pixels, 
		// have 2*BlurRadius threads sample an extra pixel.
		if (groupThreadID.x < BLUR_RADIUS)
		{
			// Clamp out of bound samples that occur at image borders.
			int x = max(dispatchThreadID.x - BLUR_RADIUS, 0);
			gCache[groupThreadID.x] = gInput[int2(x, dispatchThreadID.y)];
		}

		if (groupThreadID.x >= N - BLUR_RADIUS)
		{
			// Clamp out of bound samples that occur at image borders.
			int x = min(dispatchThreadID.x + BLUR_RADIUS, gInput.Length.x - 1);
			gCache[groupThreadID.x + 2 * BLUR_RADIUS] = gInput[int2(x, dispatchThreadID.y)];
		}

		// Clamp out of bound samples that occur at image borders.
		gCache[groupThreadID.x + BLUR_RADIUS] = gInput[min(dispatchThreadID.xy, gInput.Length.xy - 1)];

		// Wait for all threads to finish.
		GroupMemoryBarrierWithGroupSync();

		//
		// Now blur each pixel.
		//

		float blurColor = 0.0f;

		for (int i = -BLUR_RADIUS; i <= BLUR_RADIUS; ++i)
		{
			int k = groupThreadID.x + BLUR_RADIUS + i;

			blurColor += weights[BLUR_RADIUS - abs(i)] * gCache[k];
		}

		gOutput[dispatchThreadID.xy] = blurColor;
	}
	else
	{
		if (groupThreadID.x < BLUR_RADIUS)
		{
			int y = max(dispatchThreadID.x - BLUR_RADIUS, 0);
			gCache[groupThreadID.x] = gInput[int2(dispatchThreadID.y, y)];
		}
		if (groupThreadID.x >= N - BLUR_RADIUS)
		{
			int y = min(dispatchThreadID.x + BLUR_RADIUS, gInput.Length.y - 1);
			gCache[groupThreadID.x + 2 * BLUR_RADIUS] = gInput[int2(dispatchThreadID.y, y)];
		}

		gCache[groupThreadID.x + BLUR_RADIUS] = gInput[min(dispatchThreadID.yx, gInput.Length.xy - 1)];

		GroupMemoryBarrierWithGroupSync();

		float blurColor = 0.0f;

		for (int i = -BLUR_RADIUS; i <= BLUR_RADIUS; ++i)
		{
			int k = groupThreadID.x + BLUR_RADIUS + i;

			blurColor += weights[BLUR_RADIUS - abs(i)] * gCache[k];
		}

		gOutput[dispatchThreadID.yx] = blurColor;
	}
}


#endif



