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

	GRiOcclusionCullingRasterizer() {}
	GRiOcclusionCullingRasterizer(const GRiOcclusionCullingRasterizer& rhs) = delete;
	GRiOcclusionCullingRasterizer& operator=(const GRiOcclusionCullingRasterizer& rhs) = delete;
	~GRiOcclusionCullingRasterizer() = default;

	static GRiOcclusionCullingRasterizer& GetInstance();

	bool RasterizeTestBBoxSSE(GRiBoundingBox& box, __m128* matrix, float* buffer, int clientWidth, int clientHeight, float zUpperBound, float zLowerBound);

private:

	static const int sBBIndexList[36];

	__m128 SSETransformCoords(__m128 *v, __m128 *m);

	__forceinline __m128i Min(const __m128i &v0, const __m128i &v1);

	__forceinline __m128i Max(const __m128i &v0, const __m128i &v1);

	void SSEGather(SSEVFloat4 pOut[3], int triId, const __m128 xformedPos[]);

};

