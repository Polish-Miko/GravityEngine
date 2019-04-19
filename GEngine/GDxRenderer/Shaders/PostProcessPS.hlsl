

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

//Light Render Results
Texture2D gDirectLight			: register(t0);
Texture2D gAmbientLight			: register(t1);

SamplerState			basicSampler	: register(s0);
SamplerComparisonState	shadowSampler	: register(s1);


float4 main(VertexToPixel pIn) : SV_TARGET
{
	float3 direct = gDirectLight.Sample(basicSampler, pIn.uv).rgb;
	float3 ambient = gAmbientLight.Sample(basicSampler, pIn.uv).rgb;

	float directIntensity = 1.0f;
	float ambientIntensity = 1.0f;
	float3 totalColor = direct * directIntensity + ambient * ambientIntensity;

	totalColor = totalColor / (totalColor + float3(1.f, 1.f, 1.f));
	totalColor = saturate(totalColor);
	float3 gammaCorrect = lerp(totalColor, pow(totalColor, 1.0 / 2.2), 1.0f);
	return float4(gammaCorrect, 1.0f);
}