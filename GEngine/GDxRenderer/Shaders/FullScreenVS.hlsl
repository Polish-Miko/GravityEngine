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

	output.TexC = input.TexC;

	output.PosH = float4(input.PosL, 1.0f);

	// transform to homogeneous clip space.
	output.PosH.x = 2 * output.PosH.x - 1;
	output.PosH.y = 2 * output.PosH.y - 1;

	return output;
}