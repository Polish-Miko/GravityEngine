#include "stdafx.h"
#include "GRiOcclusionCullingRasterizer.h"

#include <algorithm>

typedef float Vec2[2];
typedef float Vec3[3];


/*
static const int sBBIndexList[36] =
{
	// index for top 
	4, 8, 7,
	4, 7, 3,

	// index for bottom
	5, 1, 2,
	5, 2, 6,

	// index for left
	5, 8, 4,
	5, 4, 1,

	// index for right
	2, 3, 7,
	2, 7, 6,

	// index for back
	6, 7, 8,
	6, 8, 5,

	// index for front
	1, 4, 3,
	1, 3, 2,
};
*/

//*
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
	return (c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0]);
}

bool GRiOcclusionCullingRasterizer::RasterizeTestBBoxSSE(GRiBoundingBox& box, __m128* matrix, float* buffer, int clientWidth, int clientHeight, bool bReverseZ)
{
	__m128 verticesSSE[8];

	// init vertices
	float center[3] = { box.Center[0], box.Center[1], box.Center[2] };
	float half[3] = { box.Extents[0] * 0.5f, box.Extents[1] * 0.5f, box.Extents[2] * 0.5f };

	float vMin[3] = { center[0] - half[0], center[1] - half[1], center[2] - half[2] };
	float vMax[3] = { center[0] + half[0], center[1] + half[1], center[2] + half[2] };

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
 
	// transforms
	for (int i = 0; i < 8; i++)
	{
		verticesSSE[i] = _mm_loadu_ps(vertices[i]);

		verticesSSE[i] = SSETransformCoords(&verticesSSE[i], matrix);

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
		if (xmin > clientWidth - 1 || xmax < 0 || ymin > clientHeight - 1 || ymax < 0) continue;

		// be careful xmin/xmax/ymin/ymax can be negative. Don't cast to uint32_t
		uint32_t x0 = max(int32_t(0), (int32_t)(std::floor(xmin)));
		uint32_t x1 = min(int32_t(clientWidth) - 1, (int32_t)(std::floor(xmax)));
		uint32_t y0 = max(int32_t(0), (int32_t)(std::floor(ymin)));
		uint32_t y1 = min(int32_t(clientHeight) - 1, (int32_t)(std::floor(ymax)));

		float area = edgeFunction(v0, v1, v2);

		for (uint32_t y = y0; y < y1; ++y)
		{
			for (uint32_t x = x0; x < x1; ++x)
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
					float z = 1 / oneOverZ;
					// [comment]
					// Depth-buffer test
					// [/comment]
					if ((bReverseZ && (z > buffer[y * clientWidth + x])) || (!bReverseZ && (z < buffer[y * clientWidth + x])))
					{
						//buffer[y * clientWidth + x] = z;
						return true;
					}
				}

				/*
				if (w0 >= 0 && w1 >= 0 && w2 >= 0)
				{
					w0 /= area;
					w1 /= area;
					w2 /= area;

					float curZ = 1 / (w0 / v0[2] + w1 / v1[2] + w2 / v2[2]);//是用v0[2]这个z么，而不是view depth？
					buffer[y * clientWidth + x] = curZ;
				}
				*/
			}
		}

		/*
		std::ofstream ofs;
		ofs.open("./raster2d.ppm");
		ofs << "P6\n" << w << " " << h << "\n255\n";
		ofs.write((char*)framebuffer, w * h * 3);
		ofs.close();

		delete[] framebuffer;

		return 0;
		//*/
	}
	return false;
}

/*
bool GRiOcclusionCullingRasterizer::RasterizeTestBBoxSSE(GRiBoundingBox& box, __m128* matrix, float* buffer, int clientWidth, int clientHeight, float zUpperBound, float zLowerBound)
{
	//TODO: performance
	LARGE_INTEGER frequency;        // ticks per second
	LARGE_INTEGER t1, t2;           // ticks
	double elapsedTime;

	// get ticks per second
	QueryPerformanceFrequency(&frequency);

	// start timer
	QueryPerformanceCounter(&t1);


	//verts and flags
	__m128 verticesSSE[8];
	//int flags[8];
	//static float xformedPos[3][4];
	//static int flagsLoc[3];

	// Set DAZ and FZ MXCSR bits to flush denormals to zero (i.e., make it faster)
	// Denormal are zero (DAZ) is bit 6 and Flush to zero (FZ) is bit 15. 
	// so to enable the two to have to set bits 6 and 15 which 1000 0000 0100 0000 = 0x8040
	_mm_setcsr(_mm_getcsr() | 0x8040);


	// init vertices
	//Point3F center = box.getCenter();
	//Point3F extent = box.getExtents();
	//Point4F vCenter = Point4F(center.x, center.y, center.z, 1.0);
	//Point4F vHalf = Point4F(extent.x*0.5, extent.y*0.5, extent.z*0.5, 1.0);
	float center[3] = { box.Center[0], box.Center[1], box.Center[2] };
	float half[3] = { box.Extents[0] * 0.5f, box.Extents[1] * 0.5f, box.Extents[2] * 0.5f };

	//Point4F vMin = vCenter - vHalf;
	//Point4F vMax = vCenter + vHalf;
	float vMin[3] = { center[0] - half[0], center[1] - half[1], center[2] - half[2] };
	float vMax[3] = { center[0] + half[0], center[1] + half[1], center[2] + half[2] };

	// fill vertices
	static float vertices[8][4] = {
		{vMin[0], vMin[1], vMin[2], 1.0f},
		{vMax[0], vMin[1], vMin[2], 1.0f},
		{vMax[0], vMax[1], vMin[2], 1.0f},
		{vMin[0], vMax[1], vMin[2], 1.0f},
		{vMin[0], vMin[1], vMax[2], 1.0f},
		{vMax[0], vMin[1], vMax[2], 1.0f},
		{vMax[0], vMax[1], vMax[2], 1.0f},
		{vMin[0], vMax[1], vMax[2], 1.0f}
	};
	//vertices[0] = Point4F(vMin.x, vMin.y, vMin.z, 1);
	//vertices[1] = Point4F(vMax.x, vMin.y, vMin.z, 1);
	//vertices[2] = Point4F(vMax.x, vMax.y, vMin.z, 1);
	//vertices[3] = Point4F(vMin.x, vMax.y, vMin.z, 1);
	//vertices[4] = Point4F(vMin.x, vMin.y, vMax.z, 1);
	//vertices[5] = Point4F(vMax.x, vMin.y, vMax.z, 1);
	//vertices[6] = Point4F(vMax.x, vMax.y, vMax.z, 1);
	//vertices[7] = Point4F(vMin.x, vMax.y, vMax.z, 1);

	// transforms
	for (int i = 0; i < 8; i++)
	{
		verticesSSE[i] = _mm_loadu_ps(vertices[i]);

		verticesSSE[i] = SSETransformCoords(&verticesSSE[i], matrix);

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
		const __m128 sadd = _mm_setr_ps(clientWidth * 0.5f, clientHeight * 0.5f, 0.0f, 0.0f);
		const __m128 smult = _mm_setr_ps(clientWidth * 0.5f, clientHeight * (-0.5f), 1.0f, 1.0f);

		verticesSSE[i] = _mm_add_ps(sadd, _mm_mul_ps(verticesSSE[i], smult));
	}

	// Rasterize the AABB triangles 4 at a time
	for (int i = 0; i < 12; i += 4)
	{
		SSEVFloat4 xformedPos[3];
		SSEGather(xformedPos, i, verticesSSE);

		// by 3 vertices
		// fxPtX[0] = X0 X1 X2 X3 of 1st vert in 4 triangles
		// fxPtX[1] = X0 X1 X2 X3 of 2nd vert in 4 triangles
		// and so on
		__m128i fxPtX[3], fxPtY[3];
		for (int m = 0; m < 3; m++)
		{
			fxPtX[m] = _mm_cvtps_epi32(xformedPos[m].X);
			fxPtY[m] = _mm_cvtps_epi32(xformedPos[m].Y);
		}

		// Fab(x, y) =     Ax       +       By     +      C              = 0
		// Fab(x, y) = (ya - yb)x   +   (xb - xa)y + (xa * yb - xb * ya) = 0
		// Compute A = (ya - yb) for the 3 line segments that make up each triangle
		__m128i A0 = _mm_sub_epi32(fxPtY[1], fxPtY[2]);
		__m128i A1 = _mm_sub_epi32(fxPtY[2], fxPtY[0]);
		__m128i A2 = _mm_sub_epi32(fxPtY[0], fxPtY[1]);

		// Compute B = (xb - xa) for the 3 line segments that make up each triangle
		__m128i B0 = _mm_sub_epi32(fxPtX[2], fxPtX[1]);
		__m128i B1 = _mm_sub_epi32(fxPtX[0], fxPtX[2]);
		__m128i B2 = _mm_sub_epi32(fxPtX[1], fxPtX[0]);

		// Compute C = (xa * yb - xb * ya) for the 3 line segments that make up each triangle
		__m128i C0 = _mm_sub_epi32(_mm_mullo_epi32(fxPtX[1], fxPtY[2]), _mm_mullo_epi32(fxPtX[2], fxPtY[1]));
		__m128i C1 = _mm_sub_epi32(_mm_mullo_epi32(fxPtX[2], fxPtY[0]), _mm_mullo_epi32(fxPtX[0], fxPtY[2]));
		__m128i C2 = _mm_sub_epi32(_mm_mullo_epi32(fxPtX[0], fxPtY[1]), _mm_mullo_epi32(fxPtX[1], fxPtY[0]));

		// Compute triangle area
		__m128i triArea = _mm_mullo_epi32(B2, A1);
		triArea = _mm_sub_epi32(triArea, _mm_mullo_epi32(B1, A2));
		__m128 oneOverTriArea = _mm_div_ps(_mm_set1_ps(1.0f), _mm_cvtepi32_ps(triArea));

		__m128 Z[3];
		Z[0] = xformedPos[0].W;//因为当时齐次除法除的是(w,w,w,1)，所以这里的W就是view depth。
		Z[1] = _mm_mul_ps(_mm_sub_ps(xformedPos[1].W, Z[0]), oneOverTriArea);//1点和0点的view depth之差除以三角形面积
		Z[2] = _mm_mul_ps(_mm_sub_ps(xformedPos[2].W, Z[0]), oneOverTriArea);//2点和0点的view depth之差除以三角形面积

		// Use bounding box traversal strategy to determine which pixels to rasterize 
		__m128i startX = _mm_and_si128(Max(Min(Min(fxPtX[0], fxPtX[1]), fxPtX[2]), _mm_set1_epi32(0)), _mm_set1_epi32(~1));
		__m128i endX = Min(Max(Max(fxPtX[0], fxPtX[1]), fxPtX[2]), _mm_set1_epi32(clientWidth - 1));

		__m128i startY = _mm_and_si128(Max(Min(Min(fxPtY[0], fxPtY[1]), fxPtY[2]), _mm_set1_epi32(0)), _mm_set1_epi32(~1));
		__m128i endY = Min(Max(Max(fxPtY[0], fxPtY[1]), fxPtY[2]), _mm_set1_epi32(clientHeight - 1));

		// Now we have 4 triangles set up.  Rasterize them each individually.
		for (int lane = 0; lane < 4; lane++)
		{
			// Skip triangle if area is zero 
			if (triArea.m128i_i32[lane] <= 0)
			{
				continue;
			}

			// Extract this triangle's properties from the SIMD versions
			__m128 zz[3];
			for (int vv = 0; vv < 3; vv++)
			{
				zz[vv] = _mm_set1_ps(Z[vv].m128_f32[lane]);
			}

			//drop culled triangle

			int startXx = startX.m128i_i32[lane];
			int endXx = endX.m128i_i32[lane];
			int startYy = startY.m128i_i32[lane];
			int endYy = endY.m128i_i32[lane];

			__m128i aa0 = _mm_set1_epi32(A0.m128i_i32[lane]);
			__m128i aa1 = _mm_set1_epi32(A1.m128i_i32[lane]);
			__m128i aa2 = _mm_set1_epi32(A2.m128i_i32[lane]);

			__m128i bb0 = _mm_set1_epi32(B0.m128i_i32[lane]);
			__m128i bb1 = _mm_set1_epi32(B1.m128i_i32[lane]);
			__m128i bb2 = _mm_set1_epi32(B2.m128i_i32[lane]);

			__m128i cc0 = _mm_set1_epi32(C0.m128i_i32[lane]);
			__m128i cc1 = _mm_set1_epi32(C1.m128i_i32[lane]);
			__m128i cc2 = _mm_set1_epi32(C2.m128i_i32[lane]);

			__m128i aa0Inc = _mm_mul_epi32(aa0, _mm_setr_epi32(1, 2, 3, 4));
			__m128i aa1Inc = _mm_mul_epi32(aa1, _mm_setr_epi32(1, 2, 3, 4));
			__m128i aa2Inc = _mm_mul_epi32(aa2, _mm_setr_epi32(1, 2, 3, 4));

			__m128i alpha0 = _mm_add_epi32(_mm_mul_epi32(aa0, _mm_set1_epi32(startXx)), _mm_mul_epi32(bb0, _mm_set1_epi32(startYy)));
			alpha0 = _mm_add_epi32(cc0, alpha0);
			__m128i beta0 = _mm_add_epi32(_mm_mul_epi32(aa1, _mm_set1_epi32(startXx)), _mm_mul_epi32(bb1, _mm_set1_epi32(startYy)));
			beta0 = _mm_add_epi32(cc1, beta0);
			__m128i gama0 = _mm_add_epi32(_mm_mul_epi32(aa2, _mm_set1_epi32(startXx)), _mm_mul_epi32(bb2, _mm_set1_epi32(startYy)));
			gama0 = _mm_add_epi32(cc2, gama0);

			int  rowIdx = (startYy * clientWidth + startXx);

			__m128 zx = _mm_mul_ps(_mm_cvtepi32_ps(aa1), zz[1]);
			zx = _mm_add_ps(zx, _mm_mul_ps(_mm_cvtepi32_ps(aa2), zz[2]));//aa1*zz1+aa2*zz2
			zx = _mm_mul_ps(zx, _mm_setr_ps(1.f, 2.f, 3.f, 4.f));//aa1*zz1+aa2*zz2的1,2,3,4倍

			// Texels traverse
			for (int r = startYy; r < endYy; r++,
				rowIdx += clientWidth,
				alpha0 = _mm_add_epi32(alpha0, bb0),
				beta0 = _mm_add_epi32(beta0, bb1),
				gama0 = _mm_add_epi32(gama0, bb2))
			{
				// Compute barycentric coordinates
				// Z0 as an origin
				int index = rowIdx;
				__m128i alpha = alpha0;
				__m128i beta = beta0;
				__m128i gama = gama0;

				//Compute barycentric-interpolated depth
				__m128 depth = zz[0];
				depth = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(beta), zz[1]));
				depth = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(gama), zz[2]));
				__m128i anyOut = _mm_setzero_si128();

				__m128i mask;
				__m128 previousDepth;
				__m128 depthMask;
				__m128i finalMask;
				for (int c = startXx; c < endXx;
					c += 4,
					index += 4,
					alpha = _mm_add_epi32(alpha, aa0Inc),
					beta = _mm_add_epi32(beta, aa1Inc),
					gama = _mm_add_epi32(gama, aa2Inc),
					depth = _mm_add_ps(depth, zx))
				{
					/*
					mask = _mm_or_si128(_mm_or_si128(alpha, beta), gama);
					previousDepth = _mm_loadu_ps(&(buffer[index]));

					//calculate current depth
					//(log(depth) - -6.907755375) * 0.048254941;
					for (auto ind = 0u; ind < 4; ind++)
						depth.m128_f32[ind] = log(depth.m128_f32[ind]);
					__m128 logDepth = depth;
					//__m128 curdepth = _mm_mul_ps(_mm_sub_ps(log_ps(depth), _mm_set1_ps(-6.907755375)), _mm_set1_ps(0.048254941));
					__m128 curdepth = _mm_mul_ps(_mm_sub_ps(logDepth, _mm_set1_ps(-6.907755375f)), _mm_set1_ps(0.048254941f));
					curdepth = _mm_sub_ps(curdepth, _mm_set1_ps(0.05f));

					depthMask = _mm_cmplt_ps(curdepth, previousDepth);//这个像素的当前深度小于前一帧回读的深度的话，depthMask就是全1，否则全0
					finalMask = _mm_andnot_si128(mask, _mm_castps_si128(depthMask));
					anyOut = _mm_or_si128(anyOut, finalMask);//测试是否有任何未被遮蔽的像素，只要有一个像素没被遮蔽，anyOut就不是全0
					*/
/*
					mask = _mm_or_si128(_mm_or_si128(alpha, beta), gama);
					previousDepth = _mm_loadu_ps(&(buffer[index]));

					//calculate current depth
					//(log(depth) - -6.907755375) * 0.048254941;
					for (auto ind = 0u; ind < 4; ind++)
						depth.m128_f32[ind] = log(depth.m128_f32[ind]);
					__m128 logDepth = depth;
					__m128 curdepth = _mm_mul_ps(_mm_sub_ps(logDepth, _mm_set1_ps(-6.907755375f)), _mm_set1_ps(0.048254941f));
					curdepth = _mm_sub_ps(curdepth, _mm_set1_ps(0.05f));

					depthMask = _mm_cmplt_ps(curdepth, previousDepth);//这个像素的当前深度小于前一帧回读的深度的话，depthMask就是全1，否则全0
					finalMask = _mm_andnot_si128(mask, _mm_castps_si128(depthMask));
					anyOut = _mm_or_si128(anyOut, finalMask);//测试是否有任何未被遮蔽的像素，只要有一个像素没被遮蔽，anyOut就不是全0

				}//for each column  

				if (!_mm_testz_si128(anyOut, _mm_set1_epi32(0x80000000)))//如果有任何像素没被遮蔽
				{
					// stop timer
					QueryPerformanceCounter(&t2);

					// compute and print the elapsed time in millisec
					elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;

					//RasterizationStats::RasterizeSSETimeSpent += elapsedTime;

					return true; //early exit，有未被遮蔽的像素，需要draw这个scene object
				}

			}// for each row

		}// for each triangle
	}// for each set of SIMD# triangles

	return false;//所有像素都被遮蔽了，所以这个scene object被遮蔽了，返回的是bVisible。
}
//*/

