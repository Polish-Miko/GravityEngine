
static const float PI = 3.14159265359;

TextureCube gCubeMap		: register(t0);
SamplerState basicSampler	: register(s0);

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gInvTransWorld;
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

float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates
	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// from tangent-space vector to world-space sample vector
	float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);

	float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

struct VertexOut
{
	float4 PosH		: SV_POSITION;
	float3 PosL		: POSITION;
};

float4 main(VertexOut pin) : SV_TARGET
{
	float roughness = roughnessCb;
	const uint NumSamples = 1024u;
	float3 N = normalize(pin.PosL);
	float3 R = N;
	float3 V = R;

	float3 prefilteredColor = float3(0.f, 0.f, 0.f);
	float totalWeight = 0.f;
	for (uint i = 0u; i < NumSamples; ++i)
	{
		float2 Xi = Hammersley(i, NumSamples);
		float3 H = ImportanceSampleGGX(Xi, N, roughness);
		float3 L = normalize(2.0f * dot(V, H) * H - V);
		float NdotL = max(dot(N, L), 0.0f);
		if (NdotL > 0.f)
		{
			prefilteredColor += gCubeMap.Sample(basicSampler, L).rgb * NdotL;
			totalWeight += NdotL;
		}
	}

	prefilteredColor = prefilteredColor / totalWeight;

	return float4(prefilteredColor, 1.0f);
}
