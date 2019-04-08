struct VertexInput
{
	float3 pos		: POSITION;
	float2 uv		: TEXCOORD;
	float3 normal	: NORMAL;
	float3 tangent	: TANGENT;
};

struct VertexOutput
{
	float4 pos		: SV_POSITION;
	float2 uv		: TEXCOORD;
};

cbuffer cbPerObject : register(b0)
{
	float4x4 worldViewProjection;
	float4x4 world;
	float4x4 view;
	float4x4 projection;
};

VertexOutput main(VertexInput input)
{
	VertexOutput output;
	output.pos = mul(float4(input.pos, 1.0f), worldViewProjection);
	output.uv = input.uv;
	return output;
}