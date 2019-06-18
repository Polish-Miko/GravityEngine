#include "stdafx.h"
#include "GRiOcclusionCullingRasterizer.h"

#include <algorithm>
#include <bitset>

typedef float Vec2[2];
typedef float Vec3[3];

#define COUNTER_CLOCKWISE 1

#define USE_DISCARD_HEURISTIC 0

#define USE_SSE_REPROJECT 1

#define OUTPUT_TEST 0

#define TILE_SIZE_X 32
#define TILE_SIZE_Y 4
#define SUB_TILE_SIZE_X 8
#define SUB_TILE_SIZE_Y 4
#define TILE_WIDTH_SHIFT 5
#define TILE_HEIGHT_SHIFT 2
#define SIMD_SUB_TILE_COL_OFFSET _mm_setr_epi32(0, SUB_TILE_SIZE_X, SUB_TILE_SIZE_X * 2, SUB_TILE_SIZE_X * 3)
#define SIMD_SUB_TILE_ROW_OFFSET _mm_setzero_si128()

#define QUICK_MASK 0

#define Z_IGNORE_BOUND 0.00005f
#define LAYER_BOUND 1.35f
#define TOTAL_MASK_BIT_THRESHOLD 20

#define USE_FIXED_THREAD_TASK_NUM 0



template<typename T, typename Y> __forceinline T simd_cast(Y A);
template<> __forceinline __m128  simd_cast<__m128>(float A) { return _mm_set1_ps(A); }
template<> __forceinline __m128  simd_cast<__m128>(__m128i A) { return _mm_castsi128_ps(A); }
template<> __forceinline __m128  simd_cast<__m128>(__m128 A) { return A; }
template<> __forceinline __m128i simd_cast<__m128i>(int A) { return _mm_set1_epi32(A); }
template<> __forceinline __m128i simd_cast<__m128i>(__m128 A) { return _mm_castps_si128(A); }
template<> __forceinline __m128i simd_cast<__m128i>(__m128i A) { return A; }

const int GRiOcclusionCullingRasterizer::sBBIndexList[36] =
{
	// index for top 
	3, 7, 6,
	3, 6, 2,

	// index for bottom
	4, 0, 1,
	4, 1, 5,

	// index for left
	4, 7, 3,
	4, 3, 0,

	// index for right
	1, 2, 6,
	1, 6, 5,

	// index for back
	5, 6, 7,
	5, 7, 4,
  
	// index for front
	0, 3, 2,
	0, 2, 1,
};

float min3(const float &a, const float &b, const float &c)
{
	return min(a, min(b, c));
}

float max3(const float &a, const float &b, const float &c)
{
	return max(a, max(b, c));
}

float* VecCross(Vec3 a, Vec3 b)
{
	//remember to delete ret.
	float* ret = new float[3];
	ret[0] = a[1] * b[2] - a[2] * b[1];
	ret[1] = a[2] * b[0] - a[0] * b[2];
	ret[2] = a[0] * b[1] - a[1] * b[0];
	return ret;
}

float VecDot(Vec3 a, Vec3 b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

GRiOcclusionCullingRasterizer& GRiOcclusionCullingRasterizer::GetInstance()
{
	static GRiOcclusionCullingRasterizer *instance = new GRiOcclusionCullingRasterizer();
	return *instance;
}

__m128 GRiOcclusionCullingRasterizer::SSETransformCoords(__m128 *v, __m128 *m)
{
	__m128 vResult = _mm_shuffle_ps(*v, *v, _MM_SHUFFLE(0, 0, 0, 0));
	vResult = _mm_mul_ps(vResult, m[0]);

	__m128 vTemp = _mm_shuffle_ps(*v, *v, _MM_SHUFFLE(1, 1, 1, 1));
	vTemp = _mm_mul_ps(vTemp, m[1]);

	vResult = _mm_add_ps(vResult, vTemp);
	vTemp = _mm_shuffle_ps(*v, *v, _MM_SHUFFLE(2, 2, 2, 2));

	vTemp = _mm_mul_ps(vTemp, m[2]);
	vResult = _mm_add_ps(vResult, vTemp);

	vResult = _mm_add_ps(vResult, m[3]);
	return vResult;
}

__forceinline __m128i GRiOcclusionCullingRasterizer::Min(const __m128i &v0, const __m128i &v1)
{
	__m128i tmp;
	tmp = _mm_min_epi32(v0, v1);
	return tmp;
}

__forceinline __m128i GRiOcclusionCullingRasterizer::Max(const __m128i &v0, const __m128i &v1)
{
	__m128i tmp;
	tmp = _mm_max_epi32(v0, v1);
	return tmp;
}

// get 4 triangles from vertices
void GRiOcclusionCullingRasterizer::SSEGather(SSEVFloat4 pOut[3], int triId, const __m128 xformedPos[])
{
	for (int i = 0; i < 3; i++)
	{
		int ind0 = sBBIndexList[triId * 3 + i + 0];// -1;
		int ind1 = sBBIndexList[triId * 3 + i + 3];// -1;
		int ind2 = sBBIndexList[triId * 3 + i + 6];// -1;
		int ind3 = sBBIndexList[triId * 3 + i + 9];// -1;

		__m128 v0 = xformedPos[ind0];
		__m128 v1 = xformedPos[ind1];
		__m128 v2 = xformedPos[ind2];
		__m128 v3 = xformedPos[ind3];
		_MM_TRANSPOSE4_PS(v0, v1, v2, v3);
		pOut[i].X = v0;
		pOut[i].Y = v1;
		pOut[i].Z = v2;
		pOut[i].W = v3;

		//now X contains X0 x1 x2 x3, Y - Y0 Y1 Y2 Y3 and so on...
	}
}

inline float edgeFunction(const Vec3 &a, const Vec3 &b, const Vec3 &c)
{
#if COUNTER_CLOCKWISE
	return -(c[0] - a[0]) * (b[1] - a[1]) + (c[1] - a[1]) * (b[0] - a[0]);
#else
	return (c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0]);
#endif
}

void GRiOcclusionCullingRasterizer::Reproject(float* src, float* dst, __m128* viewProj, __m128* invPrevViewProj)
{
	std::fill_n((float*)dst, mBufferWidth * mBufferHeight, 0.0f);

#if USE_SSE_REPROJECT
	__m128 readbackPos, worldPos, vertW, reprojectedPos;
	static const __m128 sign_mask = _mm_set1_ps(-0.f); // -0.f = 1 << 31
	const __m128 sadd = _mm_setr_ps(mBufferWidth * 0.5, mBufferHeight * 0.5, 0, 0);
	const __m128 smult = _mm_setr_ps(mBufferWidth * 0.5, mBufferHeight * (-0.5), 1, 1);

	for (auto i = 0u; i < mBufferWidth; i++)
	{
		for (auto j = 0u; j < mBufferHeight; j++)
		{
			readbackPos = _mm_setr_ps(((float)i / ((float)mBufferWidth - 1.0f)) * 2.0f - 1.0f, 1.0f - ((float)j / ((float)mBufferHeight - 1.0f)) * 2, src[j * mBufferWidth + i], 1.0f);

			worldPos = SSETransformCoords(&readbackPos, invPrevViewProj);

			vertW = _mm_shuffle_ps(worldPos, worldPos, _MM_SHUFFLE(3, 3, 3, 3)); // wwww
			vertW = _mm_andnot_ps(sign_mask, vertW); // abs
			worldPos = _mm_div_ps(worldPos, vertW);

			reprojectedPos = SSETransformCoords(&worldPos, viewProj);

			vertW = _mm_shuffle_ps(reprojectedPos, reprojectedPos, _MM_SHUFFLE(3, 3, 3, 3)); // wwww
			vertW = _mm_andnot_ps(sign_mask, vertW); // abs
			reprojectedPos = _mm_div_ps(reprojectedPos, vertW);

			// now vertices are between -1 and 1
			//const __m128 sadd = _mm_setr_ps(mBufferWidth * 0.5, mBufferHeight * 0.5, 0, 0);
			//const __m128 smult = _mm_setr_ps(mBufferWidth * 0.5, mBufferHeight * (-0.5), 1, 1);

			reprojectedPos = _mm_add_ps(sadd, _mm_mul_ps(reprojectedPos, smult));

			int u = (int)reprojectedPos.m128_f32[0];
			int v = (int)reprojectedPos.m128_f32[1];
			if (u >= 0 && u < mBufferWidth && v >= 0 && v < mBufferHeight)
				dst[v * mBufferWidth + u] = reprojectedPos.m128_f32[2];
		}
	}
#else
	float ipvpF[4][4] = {
		{invPrevViewProj[0].m128_f32[0], invPrevViewProj[0].m128_f32[1], invPrevViewProj[0].m128_f32[2], invPrevViewProj[0].m128_f32[3]},
		{invPrevViewProj[1].m128_f32[0], invPrevViewProj[1].m128_f32[1], invPrevViewProj[1].m128_f32[2], invPrevViewProj[1].m128_f32[3]},
		{invPrevViewProj[2].m128_f32[0], invPrevViewProj[2].m128_f32[1], invPrevViewProj[2].m128_f32[2], invPrevViewProj[2].m128_f32[3]},
		{invPrevViewProj[3].m128_f32[0], invPrevViewProj[3].m128_f32[1], invPrevViewProj[3].m128_f32[2], invPrevViewProj[3].m128_f32[3]}
	};

	float vpF[4][4] = {
		{viewProj[0].m128_f32[0], viewProj[0].m128_f32[1], viewProj[0].m128_f32[2], viewProj[0].m128_f32[3]},
		{viewProj[1].m128_f32[0], viewProj[1].m128_f32[1], viewProj[1].m128_f32[2], viewProj[1].m128_f32[3]},
		{viewProj[2].m128_f32[0], viewProj[2].m128_f32[1], viewProj[2].m128_f32[2], viewProj[2].m128_f32[3]},
		{viewProj[3].m128_f32[0], viewProj[3].m128_f32[1], viewProj[3].m128_f32[2], viewProj[3].m128_f32[3]}
	};

	float readbackPos[4] = { 0.f, 0.f, 0.f, 1.f };
	float worldPos[4] = { 0.f, 0.f, 0.f, 1.f };
	float reprojectedPos[4] = { 0.f, 0.f, 0.f, 1.f };

	for (auto i = 0u; i < mBufferWidth; i++)
	{
		for (auto j = 0u; j < mBufferHeight; j++)
		{
			readbackPos[0] = ((float)i / ((float)mBufferWidth - 1.0f)) * 2.0f - 1.0f;
			readbackPos[1] = 1.0f - ((float)j / ((float)mBufferHeight - 1.0f)) * 2.0f;
			readbackPos[2] = src[j * mBufferWidth + i];

			for (auto c = 0u; c < 4; c++)
			{
				worldPos[c] = 0;
				for (auto r = 0u; r < 4; r++)
				{
					worldPos[c] += readbackPos[r] * ipvpF[r][c];
				}
			}

			worldPos[3] = abs(worldPos[3]);
			worldPos[0] /= worldPos[3];
			worldPos[1] /= worldPos[3];
			worldPos[2] /= worldPos[3];
			worldPos[3] = 1.0f;

			for (auto c = 0u; c < 4; c++)
			{
				reprojectedPos[c] = 0;
				for (auto r = 0u; r < 4; r++)
				{
					reprojectedPos[c] += worldPos[r] * vpF[r][c];
				}
			}

			reprojectedPos[3] = abs(reprojectedPos[3]);
			reprojectedPos[0] /= reprojectedPos[3];
			reprojectedPos[1] /= reprojectedPos[3];
			reprojectedPos[2] /= reprojectedPos[3];
			reprojectedPos[3] = 1.0f;

			reprojectedPos[0] = (reprojectedPos[0] + 1.0f) * 0.5f * mBufferWidth;
			reprojectedPos[1] = (1.0f - reprojectedPos[1]) * 0.5f * mBufferHeight;

			int u = (int)reprojectedPos[0];
			int v = (int)reprojectedPos[1];
			if (u >= 0 && u < mBufferWidth && v >= 0 && v < mBufferHeight)
				dst[v * mBufferWidth + u] = reprojectedPos[2];
		}
	}
#endif
}

bool GRiOcclusionCullingRasterizer::RasterizeAndTestBBox(GRiBoundingBox& box, __m128* worldViewProj, float* buffer, float* output)
{
	__m128 verticesSSE[8];

	// to avoid self-occluding
	float expFac = 1.05f;
	float Extents[3] = { box.Extents[0] * expFac,  box.Extents[1] * expFac,  box.Extents[2] * expFac };

	float vMin[3] = { box.Center[0] - Extents[0], box.Center[1] - Extents[1], box.Center[2] - Extents[2] };
	float vMax[3] = { box.Center[0] + Extents[0], box.Center[1] + Extents[1], box.Center[2] + Extents[2] };

	// fill vertices
	float vertices[8][4] = {
		{vMin[0], vMin[1], vMin[2], 1.0f},
		{vMax[0], vMin[1], vMin[2], 1.0f},
		{vMax[0], vMax[1], vMin[2], 1.0f},
		{vMin[0], vMax[1], vMin[2], 1.0f},
		{vMin[0], vMin[1], vMax[2], 1.0f},
		{vMax[0], vMin[1], vMax[2], 1.0f},
		{vMax[0], vMax[1], vMax[2], 1.0f},
		{vMin[0], vMax[1], vMax[2], 1.0f}
	};
	bool bNearClip[8];

	// transforms
	/*
	for (int i = 0; i < 8; i++)
	{
		verticesSSE[i] = _mm_loadu_ps(vertices[i]);

		verticesSSE[i] = SSETransformCoords(&verticesSSE[i], worldViewProj);

		__m128 vertX = _mm_shuffle_ps(verticesSSE[i], verticesSSE[i], _MM_SHUFFLE(0, 0, 0, 0)); // xxxx
		__m128 vertY = _mm_shuffle_ps(verticesSSE[i], verticesSSE[i], _MM_SHUFFLE(1, 1, 1, 1)); // yyyy
		__m128 vertZ = _mm_shuffle_ps(verticesSSE[i], verticesSSE[i], _MM_SHUFFLE(2, 2, 2, 2)); // zzzz
		__m128 vertW = _mm_shuffle_ps(verticesSSE[i], verticesSSE[i], _MM_SHUFFLE(3, 3, 3, 3)); // wwww
		static const __m128 sign_mask = _mm_set1_ps(-0.f); // -0.f = 1 << 31
		vertW = _mm_andnot_ps(sign_mask, vertW); // abs
		vertW = _mm_shuffle_ps(vertW, _mm_set1_ps(1.0f), _MM_SHUFFLE(0, 0, 0, 0)); //w,w,1,1
		vertW = _mm_shuffle_ps(vertW, vertW, _MM_SHUFFLE(3, 0, 0, 0)); //w,w,w,1

		// project
		verticesSSE[i] = _mm_div_ps(verticesSSE[i], vertW);

		// now vertices are between -1 and 1
		const __m128 sadd = _mm_setr_ps(clientWidth * 0.5, clientHeight * 0.5, 0, 0);
		const __m128 smult = _mm_setr_ps(clientWidth * 0.5, clientHeight * (-0.5), 1, 1);

		verticesSSE[i] = _mm_add_ps(sadd, _mm_mul_ps(verticesSSE[i], smult));

		for (auto j = 0u; j < 3; j++)
		{
			vertices[i][j] = verticesSSE[i].m128_f32[j];
		}
		vertices[i][2] = 1 / vertices[i][2];
	}
	*/
	float wvpF[4][4] = {
		{worldViewProj[0].m128_f32[0], worldViewProj[0].m128_f32[1], worldViewProj[0].m128_f32[2], worldViewProj[0].m128_f32[3]},
		{worldViewProj[1].m128_f32[0], worldViewProj[1].m128_f32[1], worldViewProj[1].m128_f32[2], worldViewProj[1].m128_f32[3]},
		{worldViewProj[2].m128_f32[0], worldViewProj[2].m128_f32[1], worldViewProj[2].m128_f32[2], worldViewProj[2].m128_f32[3]},
		{worldViewProj[3].m128_f32[0], worldViewProj[3].m128_f32[1], worldViewProj[3].m128_f32[2], worldViewProj[3].m128_f32[3]}
	};

	float temp[8][4];

 	for (int i = 0; i < 8; i++)
	{
		bNearClip[i] = false;

		for (auto j = 0u; j < 4; j++)
		{
			temp[i][j] = 0;
			for (auto k = 0u; k < 4; k++)
			{
				temp[i][j] += vertices[i][k] * wvpF[k][j];
			}
		}

		vertices[i][0] = temp[i][0];
		vertices[i][1] = temp[i][1];
		vertices[i][2] = temp[i][2];
		vertices[i][3] = temp[i][3];

		vertices[i][0] /= abs(vertices[i][3]);
		vertices[i][1] /= abs(vertices[i][3]);
		vertices[i][2] /= abs(vertices[i][3]);
		//vertices[i][2] /= vertices[i][3];// z remains negative if the triangle is beyond near plane.

		if (vertices[i][3] < mZLowerBound)
			bNearClip[i] = true;

		vertices[i][0] = (vertices[i][0] + 1) * 0.5f * mBufferWidth;
		vertices[i][1] = (-vertices[i][1] + 1) * 0.5f * mBufferHeight;

		if(bReverseZ)
			vertices[i][2] = 1 / (1 - vertices[i][2]);
		else
			vertices[i][2] = 1 / vertices[i][2];
	}

	for (auto i = 0u; i < 36; i += 3)
	{
		Vec3 v0, v1, v2;

		v0[0] = vertices[sBBIndexList[i + 0]][0];
		v0[1] = vertices[sBBIndexList[i + 0]][1];
		v0[2] = vertices[sBBIndexList[i + 0]][2];
		v1[0] = vertices[sBBIndexList[i + 1]][0];
		v1[1] = vertices[sBBIndexList[i + 1]][1];
		v1[2] = vertices[sBBIndexList[i + 1]][2];
		v2[0] = vertices[sBBIndexList[i + 2]][0];
		v2[1] = vertices[sBBIndexList[i + 2]][1];
		v2[2] = vertices[sBBIndexList[i + 2]][2];

		float xmin = min3(v0[0], v1[0], v2[0]);
		float ymin = min3(v0[1], v1[1], v2[1]);
		float xmax = max3(v0[0], v1[0], v2[0]);
		float ymax = max3(v0[1], v1[1], v2[1]);

		// the triangle is out of screen
		if (xmin > mBufferWidth - 1 || xmax < 0 || ymin > mBufferHeight - 1 || ymax < 0)
			continue;

		bool bTriangleNearClipped = bNearClip[sBBIndexList[i + 0]] || bNearClip[sBBIndexList[i + 1]] || bNearClip[sBBIndexList[i + 2]];

		//if the bounding box is beyond the near plane, don't do near plane clip and give up occlusion culling for this object.
#if !OUTPUT_TEST
		if (bTriangleNearClipped)
			return true;
#endif

		//backface culling
		Vec3 e1, e2;
		e1[0] = v1[0] - v0[0];
		e1[1] = v1[1] - v0[1];
		e2[0] = v2[0] - v0[0];
		e2[1] = v2[1] - v0[1];
		if (e1[0] * e2[1] - e1[1] * e2[0] < 0)
			continue;

		// be careful xmin/xmax/ymin/ymax can be negative. Don't cast to uint32_t
		uint32_t x0 = max(int32_t(0), (int32_t)(std::floor(xmin)));
		uint32_t x1 = min(int32_t(mBufferWidth) - 1, (int32_t)(std::floor(xmax)));
		uint32_t y0 = max(int32_t(0), (int32_t)(std::floor(ymin)));
		uint32_t y1 = min(int32_t(mBufferHeight) - 1, (int32_t)(std::floor(ymax)));

		float area = edgeFunction(v0, v1, v2);
		
		for (uint32_t y = y0; y <= y1; ++y)
		{
			for (uint32_t x = x0; x <= x1; ++x)
			{
				Vec3 pixelSample;
				pixelSample[0] = (float)x + 0.5f;
				pixelSample[1] = (float)y + 0.5f;
				pixelSample[2] = 0.0f;
				
				float w0 = edgeFunction(v1, v2, pixelSample);
				float w1 = edgeFunction(v2, v0, pixelSample);
				float w2 = edgeFunction(v0, v1, pixelSample);
				if (w0 >= 0 && w1 >= 0 && w2 >= 0)
				{
					w0 /= area;
					w1 /= area;
					w2 /= area;
					float oneOverZ = v0[2] * w0 + v1[2] * w1 + v2[2] * w2;
					float z;
					if (bReverseZ)
						z = 1.0f - 1.0f / oneOverZ;
					else
						z = 1.0f / oneOverZ;

#if OUTPUT_TEST
					if (bTriangleNearClipped)
						output[y * clientWidth + x] = 1.0f;
					else
						output[y * clientWidth + x] = z;
#endif

					// [comment]
					// Depth-buffer test
					// [/comment]
					if ((bReverseZ && (z > buffer[y * mBufferWidth + x])) || (!bReverseZ && (z < buffer[y * mBufferWidth + x])))
					{
						return true;
						//buffer[y * clientWidth + x] = z;
					}
				}
			}
		}
	}
	return false;
}

void GRiOcclusionCullingRasterizer::ReprojectToMaskedBuffer(float* src, __m128* viewProj, __m128* invPrevViewProj)
{
	for (auto i = 0u; i < mTileNum; i++)
	{
		mMaskedDepthBuffer[i].mMask = _mm_set1_epi32(0);
		mMaskedDepthBuffer[i].mZMin[0] = _mm_set1_ps(0.0f);
		mMaskedDepthBuffer[i].mZMin[1] = _mm_set1_ps(1.0f);
	}

	/*
	__m128 readbackPos, worldPos, vertW, reprojectedPos;
	//__m128i reprojectedPosI;
	int u, v, tileU, tileV, subTileU, subTileV, mask, tileId, subTileId, SubIdInTile, tileIdX, tileIdY, subTileIdX, subTileIdY;
	static const __m128 sign_mask = _mm_set1_ps(-0.f); // -0.f = 1 << 31
	const __m128 sadd = _mm_setr_ps(mBufferWidth * 0.5, mBufferHeight * 0.5, 0, 0);
	const __m128 smult = _mm_setr_ps(mBufferWidth * 0.5, mBufferHeight * (-0.5), 1, 1);
	float dist_10, dist_t1;

	for (auto i = 0u; i < mBufferWidth; i++)
	{
		for (auto j = 0u; j < mBufferHeight; j++)
		{
			readbackPos = _mm_setr_ps(((float)i / ((float)mBufferWidth - 1.0f)) * 2.0f - 1.0f, 1.0f - ((float)j / ((float)mBufferHeight - 1.0f)) * 2, src[j * mBufferWidth + i], 1.0f);

			worldPos = SSETransformCoords(&readbackPos, invPrevViewProj);

			vertW = _mm_shuffle_ps(worldPos, worldPos, _MM_SHUFFLE(3, 3, 3, 3)); // wwww
			vertW = _mm_andnot_ps(sign_mask, vertW); // abs
			worldPos = _mm_div_ps(worldPos, vertW);

			reprojectedPos = SSETransformCoords(&worldPos, viewProj);

			vertW = _mm_shuffle_ps(reprojectedPos, reprojectedPos, _MM_SHUFFLE(3, 3, 3, 3)); // wwww
			vertW = _mm_andnot_ps(sign_mask, vertW); // abs
			reprojectedPos = _mm_div_ps(reprojectedPos, vertW);

			// now vertices are between -1 and 1
			//const __m128 sadd = _mm_setr_ps(mBufferWidth * 0.5, mBufferHeight * 0.5, 0, 0);
			//const __m128 smult = _mm_setr_ps(mBufferWidth * 0.5, mBufferHeight * (-0.5), 1, 1);

			reprojectedPos = _mm_add_ps(sadd, _mm_mul_ps(reprojectedPos, smult));
			//reprojectedPosI = _mm_cvtps_epi32(reprojectedPos);

			u = (int)reprojectedPos.m128_f32[0];
			v = (int)reprojectedPos.m128_f32[1];

			//if (u < 0 && u >= mBufferWidth && v < 0 && v >= mBufferHeight)
				//continue;

			tileU = u % TILE_SIZE_X;
			tileV = v % TILE_SIZE_Y;
			tileIdX = u / TILE_SIZE_X;
			tileIdY = v / TILE_SIZE_Y;
			tileId = tileIdY * mTileNumX + tileIdX;
			subTileU = u % SUB_TILE_SIZE_X;
			subTileV = v % SUB_TILE_SIZE_Y;
			subTileIdX = u / SUB_TILE_SIZE_X;
			subTileIdY = v / SUB_TILE_SIZE_Y;
			subTileId = subTileIdY * mSubTileNumX + subTileIdX;
			SubIdInTile = subTileId % 4;
			mask = 1 << (subTileV * 8 + subTileU);

			if (subTileIdX == 0 && subTileIdY == 2)
				mask = mask;

			if (tileIdX >= mTileNumX || tileIdX < 0 || tileIdY >= mTileNumY || tileIdY < 0 || subTileIdX >= mSubTileNumX || subTileIdX < 0 || subTileIdY >= mSubTileNumY || subTileIdY < 0)
				continue;

			//dist_10 = _mm_sub_ps(mMaskedDepthBuffer[tileId].mZMin[1], mMaskedDepthBuffer[tileId].mZMin[0]);

#if USE_DISCARD_HEURISTIC
			// Discard working layer heuristic.
			dist_10 = mMaskedDepthBuffer[tileId].mZMin[1].m128_f32[SubIdInTile] - mMaskedDepthBuffer[tileId].mZMin[0].m128_f32[SubIdInTile];
			dist_t1 = reprojectedPos.m128_f32[2] - mMaskedDepthBuffer[tileId].mZMin[1].m128_f32[SubIdInTile];
			if (dist_t1 > dist_10)
			{
				mMaskedDepthBuffer[tileId].mZMin[1].m128_f32[SubIdInTile] = 1.0f;
				mMaskedDepthBuffer[tileId].mMask.m128i_i32[SubIdInTile] = 0;
			}
#endif

			// Merge current pixel into working layertile.
			mMaskedDepthBuffer[tileId].mZMin[1].m128_f32[SubIdInTile] = min(mMaskedDepthBuffer[tileId].mZMin[1].m128_f32[SubIdInTile], reprojectedPos.m128_f32[2]);
			mMaskedDepthBuffer[tileId].mMask.m128i_i32[SubIdInTile] |= mask;

#if 0
			// Overwrite ref. layer if working layer full.
			if (mMaskedDepthBuffer[tileId].mMask.m128i_i32[SubIdInTile] == ~0)
			{
				mMaskedDepthBuffer[tileId].mZMin[0].m128_f32[SubIdInTile] = mMaskedDepthBuffer[tileId].mZMin[1].m128_f32[SubIdInTile];
				mMaskedDepthBuffer[tileId].mZMin[1].m128_f32[SubIdInTile] = 1.0f;
				mMaskedDepthBuffer[tileId].mMask.m128i_i32[SubIdInTile] = 0;
			}
#endif

			//if (u >= 0 && u < mBufferWidth && v >= 0 && v < mBufferHeight)
				//dst[v * mBufferWidth + u] = reprojectedPos.m128_f32[2];
		}
	}
	*/

	Reproject(src, mIntermediateBuffer, viewProj, invPrevViewProj);

	GenerateMaskedBuffer();

}

void GRiOcclusionCullingRasterizer::ReprojectMT(GGiThreadPool* tp, float* src, float* dst, __m128* viewProj, __m128* invPrevViewProj)
{
	std::fill_n((float*)dst, mBufferWidth * mBufferHeight, 0.0f);

	static const __m128 sign_mask = _mm_set1_ps(-0.f); // -0.f = 1 << 31
	const __m128 sadd = _mm_setr_ps(mBufferWidth * 0.5, mBufferHeight * 0.5, 0, 0);
	const __m128 smult = _mm_setr_ps(mBufferWidth * 0.5, mBufferHeight * (-0.5), 1, 1);

#if USE_FIXED_THREAD_TASK_NUM
	auto repStep = 11u;
#else
	auto repStep = (UINT32)(mBufferHeight / tp->GetThreadNum() + 1);
#endif
	for (auto ti = 0u; ti < mBufferHeight; ti += repStep)
	{
		tp->Enqueue([&, ti, src, dst, invPrevViewProj, viewProj]
		{
			__m128 readbackPos, worldPos, vertW, reprojectedPos;

			for (auto i = ti; i < mBufferHeight && i < ti + repStep; i++)
			{
				for (auto j = 0; j < mBufferWidth; j++)
				{
					readbackPos = _mm_setr_ps(((float)j / ((float)mBufferWidth - 1.0f)) * 2.0f - 1.0f, 1.0f - ((float)i / ((float)mBufferHeight - 1.0f)) * 2, src[i * mBufferWidth + j], 1.0f);

					worldPos = GRiOcclusionCullingRasterizer::SSETransformCoords(&readbackPos, invPrevViewProj);

					vertW = _mm_shuffle_ps(worldPos, worldPos, _MM_SHUFFLE(3, 3, 3, 3)); // wwww
					vertW = _mm_andnot_ps(sign_mask, vertW); // abs
					worldPos = _mm_div_ps(worldPos, vertW);

					reprojectedPos = GRiOcclusionCullingRasterizer::SSETransformCoords(&worldPos, viewProj);

					vertW = _mm_shuffle_ps(reprojectedPos, reprojectedPos, _MM_SHUFFLE(3, 3, 3, 3)); // wwww
					vertW = _mm_andnot_ps(sign_mask, vertW); // abs
					reprojectedPos = _mm_div_ps(reprojectedPos, vertW);

					// now vertices are between -1 and 1
					//const __m128 sadd = _mm_setr_ps(mBufferWidth * 0.5, mBufferHeight * 0.5, 0, 0);
					//const __m128 smult = _mm_setr_ps(mBufferWidth * 0.5, mBufferHeight * (-0.5), 1, 1);

					reprojectedPos = _mm_add_ps(sadd, _mm_mul_ps(reprojectedPos, smult));

					int u = (int)reprojectedPos.m128_f32[0];
					int v = (int)reprojectedPos.m128_f32[1];
					if (u >= 0 && u < mBufferWidth && v >= 0 && v < mBufferHeight)
					{
						//std::lock_guard<std::mutex> lock(tp->task_mutex);
						dst[v * mBufferWidth + u] = reprojectedPos.m128_f32[2];
					}
				}
			}
		}
		);
	}

	tp->Flush();
}

void GRiOcclusionCullingRasterizer::ReprojectToMaskedBufferMT(GGiThreadPool* tp, float* src, __m128* viewProj, __m128* invPrevViewProj)
{
	for (auto i = 0u; i < mTileNum; i++)
	{
		mMaskedDepthBuffer[i].mMask = _mm_set1_epi32(0);
		mMaskedDepthBuffer[i].mZMin[0] = _mm_set1_ps(0.0f);
		mMaskedDepthBuffer[i].mZMin[1] = _mm_set1_ps(1.0f);
	}

	ReprojectMT(tp, src, mIntermediateBuffer, viewProj, invPrevViewProj);

	GenerateMaskedBuffer();
}

void GRiOcclusionCullingRasterizer::GenerateMaskedBuffer()
{
	int tileIdX, tileId, subIdInTile, mask, u, v, uv;
	float depth;
	float lowerZ, upperZ;
	int lowerMask, upperMask;
	bool bLayered;
	std::bitset<32> lMaskBit, uMaskBit, totalMaskBit;

	for (auto subTileIdX = 0u; subTileIdX < mSubTileNumX; subTileIdX++)
	{

		for (auto subTileIdY = 0u; subTileIdY < mSubTileNumY; subTileIdY++)
		{

			lowerZ = 1.0f;
			upperZ = 1.0f;
			lowerMask = 0;
			upperMask = 0;
			bLayered = false;

			tileIdX = subTileIdX / 4;
			subIdInTile = subTileIdX % 4;
			tileId = subTileIdY * mTileNumX + tileIdX;

			for (auto subTileU = 0u; subTileU < 8; subTileU++)
			{
				for (auto subTileV = 0u; subTileV < 4; subTileV++)
				{

					u = subTileIdX * 8 + subTileU;
					v = subTileIdY * 4 + subTileV;
					uv = v * mBufferWidth + u;

					depth = mIntermediateBuffer[uv];
					mask = 1 << (subTileV * 8 + subTileU);

					if (depth < Z_IGNORE_BOUND)
						continue;

					if (bLayered)
					{
						if (depth < lowerZ / LAYER_BOUND)
						{
							;
						}
						else if (depth < lowerZ)
						{
							lowerZ = depth;
							lowerMask |= mask;
						}
						else if (depth > upperZ * LAYER_BOUND)
						{
							lowerZ = upperZ;
							lowerMask = upperMask;
							upperZ = depth;
							upperMask = mask;
						}
						else if (depth > upperZ)
						{
							upperMask |= mask;
						}
						// lowerZ < depth < upperZ
						else
						{
							if (upperZ - depth > depth - lowerZ)
							{
								lowerMask |= mask;
							}
							else
							{
								upperZ = depth;
								upperMask |= mask;
							}
						}
					}
					else
					{
						if (depth > lowerZ * LAYER_BOUND)
						{
							upperZ = depth;
							upperMask |= mask;
							bLayered = true;
						}
						else if (depth < lowerZ / LAYER_BOUND && lowerZ < 1.0f)
						{
							upperZ = lowerZ;
							upperMask = lowerMask;
							lowerZ = depth;
							lowerMask = mask;
							bLayered = true;
						}
						else
						{
							lowerZ = min(lowerZ, depth);
							lowerMask |= mask;
						}
					}

				}
			}

			if (bLayered)
			{
				lMaskBit = std::bitset<32>(lowerMask);
				uMaskBit = std::bitset<32>(upperMask);
				totalMaskBit = lMaskBit | uMaskBit;
				if (totalMaskBit.count() >= TOTAL_MASK_BIT_THRESHOLD)
				{
					mMaskedDepthBuffer[tileId].mZMin[0].m128_f32[subIdInTile] = lowerZ;
					mMaskedDepthBuffer[tileId].mZMin[1].m128_f32[subIdInTile] = upperZ;
					mMaskedDepthBuffer[tileId].mMask.m128i_i32[subIdInTile] = upperMask;
				}
				else if (uMaskBit.count() >= lMaskBit.count())
				{
					mMaskedDepthBuffer[tileId].mZMin[0].m128_f32[subIdInTile] = 0.0f;
					mMaskedDepthBuffer[tileId].mZMin[1].m128_f32[subIdInTile] = upperZ;
					mMaskedDepthBuffer[tileId].mMask.m128i_i32[subIdInTile] = upperMask;
				}
				else
				{
					mMaskedDepthBuffer[tileId].mZMin[0].m128_f32[subIdInTile] = 0.0f;
					mMaskedDepthBuffer[tileId].mZMin[1].m128_f32[subIdInTile] = lowerZ;
					mMaskedDepthBuffer[tileId].mMask.m128i_i32[subIdInTile] = upperMask | lowerMask;
				}
			}
			else
			{
				mMaskedDepthBuffer[tileId].mZMin[0].m128_f32[subIdInTile] = 0.0f;
				mMaskedDepthBuffer[tileId].mZMin[1].m128_f32[subIdInTile] = lowerZ;
				mMaskedDepthBuffer[tileId].mMask.m128i_i32[subIdInTile] = lowerMask;
			}

		}

	}
}

void GRiOcclusionCullingRasterizer::GenerateMaskedBufferDebugImage(float* output)
{
	int tileU, tileV, subTileU, subTileV, mask, tileId, subTileId, SubIdInTile, tileIdX, tileIdY, subTileIdX, subTileIdY, maskOut;

	for (auto i = 0u; i < mBufferWidth; i++)
	{
		for (auto j = 0u; j < mBufferHeight; j++)
		{
			tileU = i % TILE_SIZE_X;
			tileV = j % TILE_SIZE_Y;
			tileIdX = i / TILE_SIZE_X;
			tileIdY = j / TILE_SIZE_Y;
			tileId = tileIdY * mTileNumX + tileIdX;
			subTileU = i % SUB_TILE_SIZE_X;
			subTileV = j % SUB_TILE_SIZE_Y;
			subTileIdX = i / SUB_TILE_SIZE_X;
			subTileIdY = j / SUB_TILE_SIZE_Y;
			subTileId = subTileIdY * mSubTileNumX + subTileIdX;
			SubIdInTile = subTileId % 4;
			mask = 1 << (subTileV * 8 + subTileU);

			maskOut = mMaskedDepthBuffer[tileId].mMask.m128i_i32[SubIdInTile] & mask;
			if(maskOut)
				output[i + j * mBufferWidth] = mMaskedDepthBuffer[tileId].mZMin[1].m128_f32[SubIdInTile];
			else
				output[i + j * mBufferWidth] = mMaskedDepthBuffer[tileId].mZMin[0].m128_f32[SubIdInTile];
		}
	}
}

void GRiOcclusionCullingRasterizer::Init(int bufferWidth, int bufferHeight, float zLowerBound, float zUpperBound, bool reverseZ)
{
	mBufferWidth = bufferWidth;
	mBufferHeight = bufferHeight;

	mZLowerBound = zLowerBound;
	mZUpperBound = zUpperBound;

	bReverseZ = reverseZ;

	mTileNumX = (int)((bufferWidth + TILE_SIZE_X - 1) / TILE_SIZE_X);
	mTileNumY = (int)((bufferHeight + TILE_SIZE_Y - 1) / TILE_SIZE_Y);
	mTileNum = mTileNumX * mTileNumY;

	mSubTileNumX= (int)((bufferWidth + SUB_TILE_SIZE_X - 1) / SUB_TILE_SIZE_X);
	mSubTileNumY = (int)((bufferHeight + SUB_TILE_SIZE_Y - 1) / SUB_TILE_SIZE_Y);
	mSubTileNum = mSubTileNumX * mSubTileNumY;

	mMaskedDepthBuffer = new ZTile[mTileNum];
	mIntermediateBuffer = new float[mBufferWidth * mBufferHeight];
}

bool GRiOcclusionCullingRasterizer::RectTestBBoxMasked(GRiBoundingBox& box, __m128* worldViewProj)
{
	__m128 verticesSSE[8];

	// to avoid self-occluding
	float expFac = 1.05f;
	float Extents[3] = { box.Extents[0] * expFac,  box.Extents[1] * expFac,  box.Extents[2] * expFac };

	float vMin[3] = { box.Center[0] - Extents[0], box.Center[1] - Extents[1], box.Center[2] - Extents[2] };
	float vMax[3] = { box.Center[0] + Extents[0], box.Center[1] + Extents[1], box.Center[2] + Extents[2] };

	// fill vertices
	float vertices[8][4] = {
		{vMin[0], vMin[1], vMin[2], 1.0f},
		{vMax[0], vMin[1], vMin[2], 1.0f},
		{vMax[0], vMax[1], vMin[2], 1.0f},
		{vMin[0], vMax[1], vMin[2], 1.0f},
		{vMin[0], vMin[1], vMax[2], 1.0f},
		{vMax[0], vMin[1], vMax[2], 1.0f},
		{vMax[0], vMax[1], vMax[2], 1.0f},
		{vMin[0], vMax[1], vMax[2], 1.0f}
	};
	bool bNearClip[8];

	// transforms
	/*
	for (int i = 0; i < 8; i++)
	{
		verticesSSE[i] = _mm_loadu_ps(vertices[i]);

		verticesSSE[i] = SSETransformCoords(&verticesSSE[i], worldViewProj);

		__m128 vertX = _mm_shuffle_ps(verticesSSE[i], verticesSSE[i], _MM_SHUFFLE(0, 0, 0, 0)); // xxxx
		__m128 vertY = _mm_shuffle_ps(verticesSSE[i], verticesSSE[i], _MM_SHUFFLE(1, 1, 1, 1)); // yyyy
		__m128 vertZ = _mm_shuffle_ps(verticesSSE[i], verticesSSE[i], _MM_SHUFFLE(2, 2, 2, 2)); // zzzz
		__m128 vertW = _mm_shuffle_ps(verticesSSE[i], verticesSSE[i], _MM_SHUFFLE(3, 3, 3, 3)); // wwww
		static const __m128 sign_mask = _mm_set1_ps(-0.f); // -0.f = 1 << 31
		vertW = _mm_andnot_ps(sign_mask, vertW); // abs
		vertW = _mm_shuffle_ps(vertW, _mm_set1_ps(1.0f), _MM_SHUFFLE(0, 0, 0, 0)); //w,w,1,1
		vertW = _mm_shuffle_ps(vertW, vertW, _MM_SHUFFLE(3, 0, 0, 0)); //w,w,w,1

		// project
		verticesSSE[i] = _mm_div_ps(verticesSSE[i], vertW);

		// now vertices are between -1 and 1
		const __m128 sadd = _mm_setr_ps(clientWidth * 0.5, clientHeight * 0.5, 0, 0);
		const __m128 smult = _mm_setr_ps(clientWidth * 0.5, clientHeight * (-0.5), 1, 1);

		verticesSSE[i] = _mm_add_ps(sadd, _mm_mul_ps(verticesSSE[i], smult));

		for (auto j = 0u; j < 3; j++)
		{
			vertices[i][j] = verticesSSE[i].m128_f32[j];
		}
		vertices[i][2] = 1 / vertices[i][2];
	}
	*/

	float wvpF[4][4] = {
		{worldViewProj[0].m128_f32[0], worldViewProj[0].m128_f32[1], worldViewProj[0].m128_f32[2], worldViewProj[0].m128_f32[3]},
		{worldViewProj[1].m128_f32[0], worldViewProj[1].m128_f32[1], worldViewProj[1].m128_f32[2], worldViewProj[1].m128_f32[3]},
		{worldViewProj[2].m128_f32[0], worldViewProj[2].m128_f32[1], worldViewProj[2].m128_f32[2], worldViewProj[2].m128_f32[3]},
		{worldViewProj[3].m128_f32[0], worldViewProj[3].m128_f32[1], worldViewProj[3].m128_f32[2], worldViewProj[3].m128_f32[3]}
	};

	float temp[8][4];

	float maxX = 0.f,
		minX = (float)mBufferWidth,
		maxY = 0.f,
		minY = (float)mBufferHeight,
		maxZ = 0.f;

	for (int i = 0; i < 8; i++)
	{
		bNearClip[i] = false;

		for (auto j = 0u; j < 4; j++)
		{
			temp[i][j] = 0;
			for (auto k = 0u; k < 4; k++)
			{
				temp[i][j] += vertices[i][k] * wvpF[k][j];
			}
		}

		vertices[i][0] = temp[i][0];
		vertices[i][1] = temp[i][1];
		vertices[i][2] = temp[i][2];
		vertices[i][3] = temp[i][3];

		vertices[i][0] /= abs(vertices[i][3]);
		vertices[i][1] /= abs(vertices[i][3]);
		vertices[i][2] /= abs(vertices[i][3]);
		//vertices[i][2] /= vertices[i][3];// z remains negative if the triangle is beyond near plane.

		//if (vertices[i][3] < mZLowerBound)
			//bNearClip[i] = true;

		vertices[i][0] = (vertices[i][0] + 1) * 0.5f * mBufferWidth;
		vertices[i][1] = (-vertices[i][1] + 1) * 0.5f * mBufferHeight;

		/*
		if (bReverseZ)
			vertices[i][2] = 1 / (1 - vertices[i][2]);
		else
			vertices[i][2] = 1 / vertices[i][2];
		*/

		maxX = max(maxX, vertices[i][0]);
		minX = min(minX, vertices[i][0]);
		maxY = max(maxY, vertices[i][1]);
		minY = min(minY, vertices[i][1]);
		maxZ = max(maxZ, vertices[i][2]);
	}

	static const __m128i SIMD_TILE_PAD = _mm_setr_epi32(0, 32, 0, 4);
	static const __m128i SIMD_TILE_PAD_MASK = _mm_setr_epi32(~(32 - 1), ~(32 - 1), ~(4 - 1), ~(4 - 1));
	static const __m128i SIMD_SUB_TILE_PAD = _mm_setr_epi32(0, 8, 0, 4);
	static const __m128i SIMD_SUB_TILE_PAD_MASK = _mm_setr_epi32(~(8 - 1), ~(8 - 1), ~(4 - 1), ~(4 - 1));

	//////////////////////////////////////////////////////////////////////////////
	// Compute screen space bounding box and guard for out of bounds
	//////////////////////////////////////////////////////////////////////////////
	
	__m128i pixelBBoxi = _mm_cvttps_epi32(_mm_setr_ps(minX, maxX, minY, maxY));
	pixelBBoxi = _mm_max_epi32(_mm_setzero_si128(), _mm_min_epi32(_mm_setr_epi32(mBufferWidth - 1, mBufferWidth - 1, mBufferHeight - 1, mBufferHeight - 1), pixelBBoxi));

	//////////////////////////////////////////////////////////////////////////////
	// Pad bounding box to (32xN) tiles. Tile BB is used for looping / traversal
	//////////////////////////////////////////////////////////////////////////////

	__m128i tileBBoxi = _mm_and_si128(_mm_add_epi32(pixelBBoxi, SIMD_TILE_PAD), SIMD_TILE_PAD_MASK);
	int txMin = tileBBoxi.m128i_i32[0] >> TILE_WIDTH_SHIFT;
	int txMax = tileBBoxi.m128i_i32[1] >> TILE_WIDTH_SHIFT;
	int tileRowIdx = (tileBBoxi.m128i_i32[2] >> TILE_HEIGHT_SHIFT) * mTileNumX;
	int tileRowIdxEnd = (tileBBoxi.m128i_i32[3] >> TILE_HEIGHT_SHIFT) * mTileNumX;

	/*
	if (tileBBoxi.m128i_i32[0] == tileBBoxi.m128i_i32[1] || tileBBoxi.m128i_i32[2] == tileBBoxi.m128i_i32[3])
	{
		return false;
	}
	*/

	///////////////////////////////////////////////////////////////////////////////
	// Pad bounding box to (8x4) subtiles. Skip SIMD lanes outside the subtile BB
	///////////////////////////////////////////////////////////////////////////////

	__m128i subTileBBoxi = _mm_and_si128(_mm_add_epi32(pixelBBoxi, SIMD_SUB_TILE_PAD), SIMD_SUB_TILE_PAD_MASK);
	__m128i stxmin = _mm_set1_epi32(subTileBBoxi.m128i_i32[0] - 1); // - 1 to be able to use GT test
	__m128i stymin = _mm_set1_epi32(subTileBBoxi.m128i_i32[2] - 1); // - 1 to be able to use GT test
	__m128i stxmax = _mm_set1_epi32(subTileBBoxi.m128i_i32[1]);
	__m128i stymax = _mm_set1_epi32(subTileBBoxi.m128i_i32[3]);

	// Setup pixel coordinates used to discard lanes outside subtile BB
	__m128i startPixelX = _mm_add_epi32(SIMD_SUB_TILE_COL_OFFSET, _mm_set1_epi32(tileBBoxi.m128i_i32[0]));
	__m128i pixelY = _mm_add_epi32(SIMD_SUB_TILE_ROW_OFFSET, _mm_set1_epi32(tileBBoxi.m128i_i32[2]));

	//////////////////////////////////////////////////////////////////////////////
	// Compute z from w. Note that z is reversed order, 0 = far, 1 = near, which
	// means we use a greater than test, so zMax is used to test for visibility.
	//////////////////////////////////////////////////////////////////////////////
	__m128 zMax = _mm_set1_ps(maxZ);

	for (;;)
	{
		__m128i pixelX = startPixelX;
		for (int tx = txMin;;)
		{

			int tileIdx = tileRowIdx + tx;
			assert(tileIdx >= 0 && tileIdx < mTileNum);

			// Fetch zMin from masked hierarchical Z buffer
#if QUICK_MASK != 0
			__mw zBuf = mMaskedHiZBuffer[tileIdx].mZMin[0];
#else
			__m128i mask = mMaskedDepthBuffer[tileIdx].mMask;
			__m128 zMin0 =  (mMaskedDepthBuffer[tileIdx].mZMin[0], mMaskedDepthBuffer[tileIdx].mZMin[1], simd_cast<__m128>(_mm_cmpeq_epi32(mask, _mm_set1_epi32(~0))));
			__m128 zMin1 = _mm_blendv_ps(mMaskedDepthBuffer[tileIdx].mZMin[1], mMaskedDepthBuffer[tileIdx].mZMin[0], simd_cast<__m128>(_mm_cmpeq_epi32(mask, _mm_setzero_si128())));
			__m128 zBuf = _mm_min_ps(zMin0, zMin1);
#endif
			// Perform conservative greater than test against hierarchical Z buffer (zMax >= zBuf means the subtile is visible)
			__m128i zPass = simd_cast<__m128i>(_mm_cmpge_ps(zMax, zBuf));	//zPass = zMax >= zBuf ? ~0 : 0

			// Mask out lanes corresponding to subtiles outside the bounding box
			__m128i bboxTestMin = _mm_and_si128(_mm_cmpgt_epi32(pixelX, stxmin), _mm_cmpgt_epi32(pixelY, stymin));
			__m128i bboxTestMax = _mm_and_si128(_mm_cmpgt_epi32(stxmax, pixelX), _mm_cmpgt_epi32(stymax, pixelY));
			__m128i boxMask = _mm_and_si128(bboxTestMin, bboxTestMax);
			zPass = _mm_and_si128(zPass, boxMask);

			// If not all tiles failed the conservative z test we can immediately terminate the test
			if (!_mm_testz_si128(zPass, zPass))
			{
				return true;
			}

			if (++tx >= txMax)
				break;
			pixelX = _mm_add_epi32(pixelX, _mm_set1_epi32(TILE_SIZE_X));
		}

		tileRowIdx += mTileNumX;
		if (tileRowIdx >= tileRowIdxEnd)
			break;
		pixelY = _mm_add_epi32(pixelY, _mm_set1_epi32(TILE_SIZE_Y));
	}

	return false;
}

