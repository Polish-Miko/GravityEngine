
#define USE_TONE_MAPPING 1

#define USE_ACES_TONE_MAPPING 1
#define USE_UNCHARTED_TONE_MAPPING 0

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

float3 ACES(float3 color)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

float3 Uncharted2Tonemap(float3 color)
{
	const float A = 0.15;
	const float B = 0.50;
	const float C = 0.10;
	const float D = 0.20;
	const float E = 0.02;
	const float F = 0.30;

	return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}

float4 main(VertexToPixel pIn) : SV_TARGET
{

	float3 pp = gPpInput.Sample(basicSampler, pIn.uv).rgb;

#if USE_TONE_MAPPING

#if USE_ACES_TONE_MAPPING

	float3 toneMap = ACES(pp);

	float3 gammaCorrect = pow(saturate(toneMap), 1 / 2.2);
	return float4(gammaCorrect, 1.0f);

#elif USE_UNCHARTED_TONE_MAPPING

	const float White = 11.2f;
	float adaptedLum = 1.0f;
	pp *= adaptedLum;  // Exposure Adjustment

	float ExposureBias = 1.6f;
	float3 curr = Uncharted2Tonemap(ExposureBias * pp);

	float3 whiteScale = 1.0f / Uncharted2Tonemap(White);
	float3 toneMap = curr * whiteScale;

	float3 gammaCorrect = pow(saturate(toneMap), 1 / 2.2);
	return float4(gammaCorrect, 1.0f);

#else

	float3 toneMap = pp / (pp + float3(1.f, 1.f, 1.f));

	float3 gammaCorrect = pow(saturate(toneMap), 1 / 2.2);
	return float4(gammaCorrect, 1.0f);

#endif

#else
	return float4(pp, 1.0f);
#endif

}

