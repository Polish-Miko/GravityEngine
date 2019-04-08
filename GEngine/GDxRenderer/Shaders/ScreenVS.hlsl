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

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gTexTransform;
	uint gMaterialIndex;
	uint gObjPad0;
	uint gObjPad1;
	uint gObjPad2;
};

VertexOut main(VertexIn input)
{
	VertexOut output;

	output.PosH = mul(float4(input.PosL, 1.0f), gWorld);

	// transform to homogeneous clip space.
	output.PosH.x = 2 * output.PosH.x - 1;
	output.PosH.y = 2 * output.PosH.y - 1;

	output.TexC = input.TexC;

	return output;
}