
#include "ObjectCB.hlsli"
#include "SkyPassCB.hlsli"



TextureCube gCubeMap : register(t0);
SamplerState basicSampler	: register(s0);

struct PixelOutput
{
	float4	color						: SV_TARGET0;
	float2	velocity					: SV_TARGET1;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float3 PosL		:	POSITION0;
	float4 curPos	:	POSITION1;
	float4 prevPos	:	POSITION2;
};

PixelOutput main(VertexOut pin)// : SV_TARGET
{
	float4 prevPos = pin.prevPos;
	prevPos = prevPos / prevPos.w;
	float4 curPos = pin.curPos;
	curPos = curPos / curPos.w;

	PixelOutput output;
	float3 color = gCubeMap.Sample(basicSampler, pin.PosL).rgb;
	//color = color / (float3(1.0f, 1.0f, 1.0f) + color);
	//color = pow(color, (1 / 2.2f));
	output.color = float4(color, 1.0f);
	output.velocity = float2(curPos.x - prevPos.x, curPos.y - prevPos.y);
	return output;
	//return float4(1.0f, 0.0f, 0.0f, 1.0f);
}

