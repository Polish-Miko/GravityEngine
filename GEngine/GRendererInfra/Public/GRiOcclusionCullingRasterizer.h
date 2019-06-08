#pragma once
#include "GRiPreInclude.h"
#include "GRiBoundingBox.h"





struct SSEVFloat4
{
	__m128 X;
	__m128 Y;
	__m128 Z;
	__m128 W;
};

class GRiOcclusionCullingRasterizer
{

public:

	GRiOcclusionCullingRasterizer(const GRiOcclusionCullingRasterizer& rhs) = delete;
	GRiOcclusionCullingRasterizer& operator=(const GRiOcclusionCullingRasterizer& rhs) = delete;
	~GRiOcclusionCullingRasterizer() = default;

	static GRiOcclusionCullingRasterizer& GetInstance();

	void Reproject(float* src, float* dst, __m128* viewProj, __m128* invPrevViewProj, int bufferWidth, int bufferHeight);

	bool RasterizeTestBBoxSSE(GRiBoundingBox& box, __m128* worldViewProj, float* buffer, float* output, int clientWidth, int clientHeight, float zLowerBound, float zUpperBound, bool bReverseZ);

private:

	GRiOcclusionCullingRasterizer() {}

	static const int sBBIndexList[36];

	__m128 SSETransformCoords(__m128 *v, __m128 *m);

	__forceinline __m128i Min(const __m128i &v0, const __m128i &v1);

	__forceinline __m128i Max(const __m128i &v0, const __m128i &v1);

	void SSEGather(SSEVFloat4 pOut[3], int triId, const __m128 xformedPos[]);

};

