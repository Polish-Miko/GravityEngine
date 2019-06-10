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

struct ZTile
{
	__m128        mZMin[2];
	__m128i       mMask;
};

class GRiOcclusionCullingRasterizer
{

public:

	GRiOcclusionCullingRasterizer(const GRiOcclusionCullingRasterizer& rhs) = delete;
	GRiOcclusionCullingRasterizer& operator=(const GRiOcclusionCullingRasterizer& rhs) = delete;
	~GRiOcclusionCullingRasterizer() = default;

	static GRiOcclusionCullingRasterizer& GetInstance();

	void Init(int bufferWidth, int bufferHeight, float zLowerBound, float zUpperBound, bool reverseZ);

	void Reproject(float* src, float* dst, __m128* viewProj, __m128* invPrevViewProj);

	void ReprojectToMaskedBuffer(float* src, __m128* viewProj, __m128* invPrevViewProj);

	bool RasterizeTestBBoxSSE(GRiBoundingBox& box, __m128* worldViewProj, float* buffer, float* output);

	void GenerateMaskedBufferDebugImage(float* output);

private:

	int mBufferWidth = 0;
	int mBufferHeight = 0;

	int mTileNumX = 0;
	int mTileNumY = 0;
	int mTileNum = 0;

	int mSubTileNumX = 0;
	int mSubTileNumY = 0;
	int mSubTileNum = 0;

	float mZLowerBound = 0.0f;
	float mZUpperBound = 0.0f;

	bool bReverseZ = true;

	ZTile* mMaskedDepthBuffer;

	GRiOcclusionCullingRasterizer() {}

	static const int sBBIndexList[36];

	__m128 SSETransformCoords(__m128 *v, __m128 *m);

	__forceinline __m128i Min(const __m128i &v0, const __m128i &v1);

	__forceinline __m128i Max(const __m128i &v0, const __m128i &v1);

	void SSEGather(SSEVFloat4 pOut[3], int triId, const __m128 xformedPos[]);

};

