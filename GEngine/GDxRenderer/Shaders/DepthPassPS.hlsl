
#include "ObjectCB.hlsli"
#include "MainPassCB.hlsli"

//#define TEST 1

struct VertexOutput
{
	float4	pos			: SV_POSITION;
	float4  cPos		: POSITION0;
	//float4  jcPos		: POSITION1;
};

SamplerState Sampler	   : register(s0);


float main(VertexOutput input) : SV_TARGET
{

	double4 cPos = input.cPos;
	//cPos = cPos / cPos.w;
	//cPos.xy = (cPos.xy + float2(1.0f, 1.0f)) / float2(2.0f, 2.0f);

#ifdef TEST
	return (double)((-cPos.y / cPos.w + 1) / 2);
#endif
	double depth = (double)((cPos.z - gNearZ) / gFarZ);

	return (float)depth;

}

