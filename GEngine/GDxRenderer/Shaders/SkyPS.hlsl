
TextureCube gCubeMap : register(t0);
SamplerState basicSampler	: register(s0);

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gTexTransform;
	uint gMaterialIndex;
	uint gObjPad0;
	uint gObjPad1;
	uint gObjPad2;
};

// Constant data that varies per material.
cbuffer cbPass : register(b1)
{
	float4x4 gViewProj;
	float3 gEyePosW;
	float roughnessCb;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};

float4 main(VertexOut pin) : SV_TARGET
{
	float3 color = gCubeMap.Sample(basicSampler, pin.PosL).rgb;
	//color = color / (float3(1.0f, 1.0f, 1.0f) + color);
	//color = pow(color, (1 / 2.2f));
	return float4(color, 1.0f);
	//return float4(1.0f, 0.0f, 0.0f, 1.0f);
}

