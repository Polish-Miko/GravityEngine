
#include "StaticSamplers.hlsli"
#include "ShaderDefinition.h"


struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

static const float weight[9] =
{
	1.0f / 16,
	1.0f / 8,
	1.0f / 16,
	1.0f / 8,
	1.0f / 4,
	1.0f / 8,
	1.0f / 16,
	1.0f / 8,
	1.0f / 16
};

// Input textures.
Texture2D gInputTexture					: register(t0);

float4 main(VertexToPixel i) : SV_Target
{
	int3 offset = int3(-1, 0, 1);

	float3 color = gInputTexture.SampleLevel(linearClampSampler, i.uv, 0.0f, offset.xx).rgb * weight[0]
		+ gInputTexture.SampleLevel(linearClampSampler, i.uv, 0.0f, offset.xy).rgb * weight[1]
		+ gInputTexture.SampleLevel(linearClampSampler, i.uv, 0.0f, offset.xz).rgb * weight[2]
		+ gInputTexture.SampleLevel(linearClampSampler, i.uv, 0.0f, offset.yx).rgb * weight[3]
		+ gInputTexture.SampleLevel(linearClampSampler, i.uv, 0.0f, offset.yy).rgb * weight[4]
		+ gInputTexture.SampleLevel(linearClampSampler, i.uv, 0.0f, offset.yz).rgb * weight[5]
		+ gInputTexture.SampleLevel(linearClampSampler, i.uv, 0.0f, offset.zx).rgb * weight[6]
		+ gInputTexture.SampleLevel(linearClampSampler, i.uv, 0.0f, offset.zy).rgb * weight[7]
		+ gInputTexture.SampleLevel(linearClampSampler, i.uv, 0.0f, offset.zz).rgb * weight[8];

	return float4(color, 1.0f);
}

