
#include "HaltonSequence.hlsli"
#include "MainPassCB.hlsli"

struct VertexIn
{
	float3 PosL    : POSITION;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float2 TexC    : TEXCOORD;
};

VertexOut main(VertexIn input)
{
	VertexOut output;

	uint subsampIndex = gFrameCount % SAMPLE_COUNT;
	float2 Jitter = float2(
		Halton_2_3[subsampIndex].x * gInvRenderTargetSize.x * JitterDistance,
		Halton_2_3[subsampIndex].y * gInvRenderTargetSize.y * JitterDistance
		) / 2;

	output.TexC = input.TexC;

	output.PosH = float4(input.PosL, 1.0f);
	output.PosH -= float4(Jitter, 0.0f, 0.0f);

	// transform to homogeneous clip space.
	output.PosH.x = 2 * output.PosH.x - 1;
	output.PosH.y = 2 * output.PosH.y - 1;

	return output;
}