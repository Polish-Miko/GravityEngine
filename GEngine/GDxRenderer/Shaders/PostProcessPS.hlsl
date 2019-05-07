
//#define TEST

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

//Light Render Results
//Texture2D gDirectLight			: register(t0);
//Texture2D gAmbientLight			: register(t1);
Texture2D gPpInput					: register(t0);
//Texture2D gSkyPass				: register(t1);

SamplerState			basicSampler	: register(s0);
SamplerComparisonState	shadowSampler	: register(s1);


float4 main(VertexToPixel pIn) : SV_TARGET
{

	float3 pp = gPpInput.Sample(basicSampler, pIn.uv).rgb;

#ifdef TEST
	return float4(pp, 1.0f);
#else
	float3 toneMap = pp / (pp + float3(1.f, 1.f, 1.f));
	toneMap = saturate(toneMap);

	float3 totalColor = toneMap;

	float3 gammaCorrect = lerp(totalColor, pow(totalColor, 1.0 / 2.2), 1.0f);
	return float4(gammaCorrect, 1.0f);
#endif

}

