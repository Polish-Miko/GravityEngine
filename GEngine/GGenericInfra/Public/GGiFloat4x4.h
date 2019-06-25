#pragma once
#include "GGiPreInclude.h"
#include "GGiException.h"
#include "GGiMath.h"


using namespace GGiMath;



static const __m128 GGiIdentityRow0 = { 1.0f, 0.0f, 0.0f, 0.0f };
static const __m128 GGiIdentityRow1 = { 0.0f, 1.0f, 0.0f, 0.0f };
static const __m128 GGiIdentityRow2 = { 0.0f, 0.0f, 1.0f, 0.0f };
static const __m128 GGiIdentityRow3 = { 0.0f, 0.0f, 0.0f, 1.0f };


class GGiFloat4x4
{

public:

	GGiFloat4x4();

	~GGiFloat4x4();

private:

	__m128 row[4];

public:

	inline float GetElement
	(
		int i,
		int j
	)
	{
		if (i < 0 || i > 3 || j < 0 || j > 3)
			ThrowGGiException("GGiFloat4x4 index error");
		
		return row[i].m128_f32[j];
	}

	inline void SetElement
	(
		int i,
		int j,
		float value
	)
	{
		if (i < 0 || i > 3 || j < 0 || j > 3)
			ThrowGGiException("GGiFloat4x4 index error");

		row[i].m128_f32[j] = value;
	}

	inline GGiFloat4x4& operator =(GGiFloat4x4& mat)
	{
		row[0] = mat.row[0];
		row[1] = mat.row[1];
		row[2] = mat.row[2];
		row[3] = mat.row[3];
		return *this;
	}

	__forceinline GGiFloat4x4& operator *(GGiFloat4x4& mat)
	{
		GGiFloat4x4 mResult;

		// Use vW to hold the original row
		__m128 vW = row[0];
		__m128 vX = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 vY = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(1, 1, 1, 1));
		__m128 vZ = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(2, 2, 2, 2));
		vW = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(3, 3, 3, 3));

		// Perform the operation on the first row
		vX = _mm_mul_ps(vX, mat.row[0]);
		vY = _mm_mul_ps(vY, mat.row[1]);
		vZ = _mm_mul_ps(vZ, mat.row[2]);
		vW = _mm_mul_ps(vW, mat.row[3]);
		// Perform a binary add to reduce cumulative errors
		vX = _mm_add_ps(vX, vZ);
		vY = _mm_add_ps(vY, vW);
		vX = _mm_add_ps(vX, vY);
		mResult.row[0] = vX;

		// Repeat for the other 3 rows
		vW = row[1];
		vX = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(0, 0, 0, 0));
		vY = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(1, 1, 1, 1));
		vZ = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(2, 2, 2, 2));
		vW = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(3, 3, 3, 3));

		vX = _mm_mul_ps(vX, mat.row[0]);
		vY = _mm_mul_ps(vY, mat.row[1]);
		vZ = _mm_mul_ps(vZ, mat.row[2]);
		vW = _mm_mul_ps(vW, mat.row[3]);
		vX = _mm_add_ps(vX, vZ);
		vY = _mm_add_ps(vY, vW);
		vX = _mm_add_ps(vX, vY);
		mResult.row[1] = vX;

		vW = row[2];
		vX = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(0, 0, 0, 0));
		vY = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(1, 1, 1, 1));
		vZ = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(2, 2, 2, 2));
		vW = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(3, 3, 3, 3));

		vX = _mm_mul_ps(vX, mat.row[0]);
		vY = _mm_mul_ps(vY, mat.row[1]);
		vZ = _mm_mul_ps(vZ, mat.row[2]);
		vW = _mm_mul_ps(vW, mat.row[3]);
		vX = _mm_add_ps(vX, vZ);
		vY = _mm_add_ps(vY, vW);
		vX = _mm_add_ps(vX, vY);
		mResult.row[2] = vX;

		vW = row[3];
		vX = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(0, 0, 0, 0));
		vY = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(1, 1, 1, 1));
		vZ = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(2, 2, 2, 2));
		vW = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(3, 3, 3, 3));

		vX = _mm_mul_ps(vX, mat.row[0]);
		vY = _mm_mul_ps(vY, mat.row[1]);
		vZ = _mm_mul_ps(vZ, mat.row[2]);
		vW = _mm_mul_ps(vW, mat.row[3]);
		vX = _mm_add_ps(vX, vZ);
		vY = _mm_add_ps(vY, vW);
		vX = _mm_add_ps(vX, vY);
		mResult.row[3] = vX;

		return mResult;
	}

	inline GGiFloat4x4& operator +(GGiFloat4x4& mat)
	{
		GGiFloat4x4 mResult;

		mResult.row[0] = _mm_add_ps(row[0], mat.row[0]);
		mResult.row[1] = _mm_add_ps(row[1], mat.row[1]);
		mResult.row[2] = _mm_add_ps(row[2], mat.row[2]);
		mResult.row[3] = _mm_add_ps(row[3], mat.row[3]);

		return mResult;
	}

	inline void SetByTranslation
	(
		float x, 
		float y, 
		float z
	)
	{
		row[0] = GGiIdentityRow0;
		row[1] = GGiIdentityRow1;
		row[2] = GGiIdentityRow2;
		row[3] = _mm_set_ps(1.f, z, y, x); 
	}

	static inline GGiFloat4x4 GetMatrixByTranslation
	(
		float x, 
		float y, 
		float z
	)
	{
		GGiFloat4x4 M;

		M.SetByRotationPitchYawRoll(x, y, z);

		return M;
	}

	inline void SetByRotationPitchYawRoll
	(
		float pitch, 
		float yaw, 
		float roll
	)
	{
		//GGiVector4 Angles = XMVectorSet(Pitch, Yaw, Roll, 0.0f); 
		GGiVector4 Angles = GGiVectorSet(pitch, yaw, roll, 0.0f);
		GGiVector4 Quaternion = GGiMathHelper::QuaternionRotationFromPitchYawRollVector(Angles);

		static const __m128 Constant1110 = { 1.0f, 1.0f, 1.0f, 0.0f };

		GGiVector4 Q0 = _mm_add_ps(Quaternion, Quaternion);
		GGiVector4 Q1 = _mm_mul_ps(Quaternion, Q0);

		GGiVector4 V0 = _GGI_SHUFFLE_PS(Q1, _MM_SHUFFLE(3, 0, 0, 1));
		V0 = _mm_and_ps(V0, g_GGiMask3);
		GGiVector4 V1 = _GGI_SHUFFLE_PS(Q1, _MM_SHUFFLE(3, 1, 2, 2));
		V1 = _mm_and_ps(V1, g_GGiMask3);
		GGiVector4 R0 = _mm_sub_ps(Constant1110, V0);
		R0 = _mm_sub_ps(R0, V1);

		V0 = _GGI_SHUFFLE_PS(Quaternion, _MM_SHUFFLE(3, 1, 0, 0));
		V1 = _GGI_SHUFFLE_PS(Q0, _MM_SHUFFLE(3, 2, 1, 2));
		V0 = _mm_mul_ps(V0, V1);

		V1 = _GGI_SHUFFLE_PS(Quaternion, _MM_SHUFFLE(3, 3, 3, 3));
		GGiVector4 V2 = _GGI_SHUFFLE_PS(Q0, _MM_SHUFFLE(3, 0, 2, 1));
		V1 = _mm_mul_ps(V1, V2);

		GGiVector4 R1 = _mm_add_ps(V0, V1);
		GGiVector4 R2 = _mm_sub_ps(V0, V1);

		V0 = _mm_shuffle_ps(R1, R2, _MM_SHUFFLE(1, 0, 2, 1));
		V0 = _GGI_SHUFFLE_PS(V0, _MM_SHUFFLE(1, 3, 2, 0));
		V1 = _mm_shuffle_ps(R1, R2, _MM_SHUFFLE(2, 2, 0, 0));
		V1 = _GGI_SHUFFLE_PS(V1, _MM_SHUFFLE(2, 0, 2, 0));

		Q1 = _mm_shuffle_ps(R0, V0, _MM_SHUFFLE(1, 0, 3, 0));
		Q1 = _GGI_SHUFFLE_PS(Q1, _MM_SHUFFLE(1, 3, 2, 0));

		/*
		GGiFloat4x4 M;
		M.row[0] = Q1;

		Q1 = _mm_shuffle_ps(R0, V0, _MM_SHUFFLE(3, 2, 3, 1));
		Q1 = _GGI_SHUFFLE_PS(Q1, _MM_SHUFFLE(1, 3, 0, 2));
		M.row[1] = Q1;

		Q1 = _mm_shuffle_ps(V1, R0, _MM_SHUFFLE(3, 2, 1, 0));
		M.row[2] = Q1;
		M.row[3] = g_GGiIdentityR3;
		return M;
		*/

		row[0] = Q1;

		Q1 = _mm_shuffle_ps(R0, V0, _MM_SHUFFLE(3, 2, 3, 1));
		Q1 = _GGI_SHUFFLE_PS(Q1, _MM_SHUFFLE(1, 3, 0, 2));
		row[1] = Q1;

		Q1 = _mm_shuffle_ps(V1, R0, _MM_SHUFFLE(3, 2, 1, 0));
		row[2] = Q1;
		row[3] = g_GGiIdentityR3;
	}

	static inline GGiFloat4x4 GetMatrixByRotationPitchYawRoll
	(
		float pitch,
		float yaw,
		float roll
	)
	{
		GGiFloat4x4 M;

		M.SetByRotationPitchYawRoll(pitch, yaw, roll);

		return M;
	}

	inline void SetByScale
	(
		float x,
		float y, 
		float z
	)
	{
		row[0] = _mm_set_ps(0, 0, 0, x);
		row[1] = _mm_set_ps(0, 0, y, 0);
		row[2] = _mm_set_ps(0, z, 0, 0);
		row[3] = g_GGiIdentityR3;
	}

	static inline GGiFloat4x4 GetMatrixByScale
	(
		float x,
		float y,
		float z
	)
	{
		GGiFloat4x4 M;

		M.SetByScale(x, y, z);

		return M;
	}

	inline void _GGI_VECTOR_CALL SetByPerspectiveFovLH
	(
		float FovAngleY,
		float AspectRatio,
		float NearZ,
		float FarZ
	)
	{
		assert(NearZ > 0.f && FarZ > 0.f);
		assert(!GGiScalarNearEqual(FovAngleY, 0.0f, 0.00001f * 2.0f));
		assert(!GGiScalarNearEqual(AspectRatio, 0.0f, 0.00001f));
		assert(!GGiScalarNearEqual(FarZ, NearZ, 0.00001f));

		float    SinFov;
		float    CosFov;
		GGiMathHelper::GGiScalarSinCos(&SinFov, &CosFov, 0.5f * FovAngleY);

		float fRange = FarZ / (FarZ - NearZ);
		// Note: This is recorded on the stack
		float Height = CosFov / SinFov;
		GGiVector4 rMem = {
			Height / AspectRatio,
			Height,
			fRange,
			-fRange * NearZ
		};
		// Copy from memory to SSE register
		GGiVector4 vValues = rMem;
		GGiVector4 vTemp = _mm_setzero_ps();
		// Copy x only
		vTemp = _mm_move_ss(vTemp, vValues);
		// CosFov / SinFov,0,0,0

		row[0] = vTemp;
		// 0,Height / AspectRatio,0,0
		vTemp = vValues;
		vTemp = _mm_and_ps(vTemp, g_GGiMaskY);
		row[1] = vTemp;
		// x=fRange,y=-fRange * NearZ,0,1.0f
		vTemp = _mm_setzero_ps();
		vValues = _mm_shuffle_ps(vValues, g_GGiIdentityR3, _MM_SHUFFLE(3, 2, 3, 2));
		// 0,0,fRange,1.0f
		vTemp = _mm_shuffle_ps(vTemp, vValues, _MM_SHUFFLE(3, 0, 0, 0));
		row[2] = vTemp;
		// 0,0,-fRange * NearZ,0.0f
		vTemp = _mm_shuffle_ps(vTemp, vValues, _MM_SHUFFLE(2, 1, 0, 0));
		row[3] = vTemp;
	}

	static inline GGiFloat4x4 _GGI_VECTOR_CALL GetMatrixByPerspectiveFovLH
	(
		float FovAngleY,
		float AspectRatio,
		float NearZ,
		float FarZ
	)
	{
		GGiFloat4x4 M;

		M.SetByPerspectiveFovLH(FovAngleY, AspectRatio, NearZ, FarZ);

		return M;
	}

	static inline GGiFloat4x4 _GGI_VECTOR_CALL GGiMatrixRotationNormal
	(
		GGiVector4_F NormalAxis,
		float     Angle
	)
	{
		float    fSinAngle;
		float    fCosAngle;
		GGiMathHelper::GGiScalarSinCos(&fSinAngle, &fCosAngle, Angle);

		GGiVector4 C2 = _mm_set_ps1(1.0f - fCosAngle);
		GGiVector4 C1 = _mm_set_ps1(fCosAngle);
		GGiVector4 C0 = _mm_set_ps1(fSinAngle);

		GGiVector4 N0 = _GGI_SHUFFLE_PS(NormalAxis, _MM_SHUFFLE(3, 0, 2, 1));
		GGiVector4 N1 = _GGI_SHUFFLE_PS(NormalAxis, _MM_SHUFFLE(3, 1, 0, 2));

		GGiVector4 V0 = _mm_mul_ps(C2, N0);
		V0 = _mm_mul_ps(V0, N1);

		GGiVector4 R0 = _mm_mul_ps(C2, NormalAxis);
		R0 = _mm_mul_ps(R0, NormalAxis);
		R0 = _mm_add_ps(R0, C1);

		GGiVector4 R1 = _mm_mul_ps(C0, NormalAxis);
		R1 = _mm_add_ps(R1, V0);
		GGiVector4 R2 = _mm_mul_ps(C0, NormalAxis);
		R2 = _mm_sub_ps(V0, R2);

		V0 = _mm_and_ps(R0, g_GGiMask3);
		GGiVector4 V1 = _mm_shuffle_ps(R1, R2, _MM_SHUFFLE(2, 1, 2, 0));
		V1 = _GGI_SHUFFLE_PS(V1, _MM_SHUFFLE(0, 3, 2, 1));
		GGiVector4 V2 = _mm_shuffle_ps(R1, R2, _MM_SHUFFLE(0, 0, 1, 1));
		V2 = _GGI_SHUFFLE_PS(V2, _MM_SHUFFLE(2, 0, 2, 0));

		R2 = _mm_shuffle_ps(V0, V1, _MM_SHUFFLE(1, 0, 3, 0));
		R2 = _GGI_SHUFFLE_PS(R2, _MM_SHUFFLE(1, 3, 2, 0));

		GGiFloat4x4 M;
		M.row[0] = R2;

		R2 = _mm_shuffle_ps(V0, V1, _MM_SHUFFLE(3, 2, 3, 1));
		R2 = _GGI_SHUFFLE_PS(R2, _MM_SHUFFLE(1, 3, 0, 2));
		M.row[1] = R2;

		V2 = _mm_shuffle_ps(V2, V0, _MM_SHUFFLE(3, 2, 1, 0));
		M.row[2] = V2;
		M.row[3] = g_GGiIdentityR3;
		return M;
	}

	inline void _GGI_VECTOR_CALL SetByRotationAxis
	(
		float axisX,
		float axisY,
		float axisZ, 
		float angle
	)
	{
		auto Axis = GGiLoadFloat3(axisX, axisY, axisZ);

		assert(!GGiVector3Equal(Axis, GGiVectorZero()));
		assert(!GGiVector3IsInfinite(Axis));

		GGiVector4 Normal = GGiVector3Normalize(Axis);

		auto M = GGiMatrixRotationNormal(Normal, angle);

		row[0] = M.row[0];
		row[1] = M.row[1];
		row[2] = M.row[2];
		row[3] = M.row[3];
	}

	static inline GGiFloat4x4 _GGI_VECTOR_CALL GetMatrixByRotationAxis
	(
		float axisX,
		float axisY,
		float axisZ,
		float angle
	)
	{
		GGiFloat4x4 M;

		M.SetByRotationAxis(axisX, axisY, axisZ, angle);

		return M;
	}

	inline void _GGI_VECTOR_CALL SetByRotationY
	(
		float Angle
	)
	{
		float    SinAngle;
		float    CosAngle;
		GGiMathHelper::GGiScalarSinCos(&SinAngle, &CosAngle, Angle);

		GGiVector4 vSin = _mm_set_ss(SinAngle);
		GGiVector4 vCos = _mm_set_ss(CosAngle);
		// x = sin,y = 0,z = cos, w = 0
		vSin = _mm_shuffle_ps(vSin, vCos, _MM_SHUFFLE(3, 0, 3, 0));

		row[2] = vSin;
		row[1] = g_GGiIdentityR1;
		// x = cos,y = 0,z = sin, w = 0
		vSin = _GGI_SHUFFLE_PS(vSin, _MM_SHUFFLE(3, 0, 1, 2));
		// x = cos,y = 0,z = -sin, w = 0
		vSin = _mm_mul_ps(vSin, g_GGiNegateZ);
		row[0] = vSin;
		row[3] = g_GGiIdentityR3;
	}

	static inline GGiFloat4x4 _GGI_VECTOR_CALL GetMatrixByRotationY
	(
		float Angle
	)
	{
		GGiFloat4x4 M;

		M.SetByRotationY(Angle);

		return M;
	}

	static inline GGiVector4 _GGI_VECTOR_CALL TransformNormal
	(
		GGiVector4_F V,
		GGiFloat4x4 M
	)
	{
		GGiVector4 vResult = _GGI_SHUFFLE_PS(V, _MM_SHUFFLE(0, 0, 0, 0));
		vResult = _mm_mul_ps(vResult, M.row[0]);
		GGiVector4 vTemp = _GGI_SHUFFLE_PS(V, _MM_SHUFFLE(1, 1, 1, 1));
		vTemp = _mm_mul_ps(vTemp, M.row[1]);
		vResult = _mm_add_ps(vResult, vTemp);
		vTemp = _GGI_SHUFFLE_PS(V, _MM_SHUFFLE(2, 2, 2, 2));
		vTemp = _mm_mul_ps(vTemp, M.row[2]);
		vResult = _mm_add_ps(vResult, vTemp);
		return vResult;
	}

	inline GGiVector4 _GGI_VECTOR_CALL TransformNormal
	(
		GGiVector4_F V
	)
	{
		GGiVector4 vResult = _GGI_SHUFFLE_PS(V, _MM_SHUFFLE(0, 0, 0, 0));
		vResult = _mm_mul_ps(vResult, row[0]);
		GGiVector4 vTemp = _GGI_SHUFFLE_PS(V, _MM_SHUFFLE(1, 1, 1, 1));
		vTemp = _mm_mul_ps(vTemp, row[1]);
		vResult = _mm_add_ps(vResult, vTemp);
		vTemp = _GGI_SHUFFLE_PS(V, _MM_SHUFFLE(2, 2, 2, 2));
		vTemp = _mm_mul_ps(vTemp, row[2]);
		vResult = _mm_add_ps(vResult, vTemp);
		return vResult;
	}

	static inline GGiFloat4x4 _GGI_VECTOR_CALL Transpose
	(
		GGiFloat4x4 M
	)
	{
		// x.x,x.y,y.x,y.y
		GGiVector4 vTemp1 = _mm_shuffle_ps(M.row[0], M.row[1], _MM_SHUFFLE(1, 0, 1, 0));
		// x.z,x.w,y.z,y.w
		GGiVector4 vTemp3 = _mm_shuffle_ps(M.row[0], M.row[1], _MM_SHUFFLE(3, 2, 3, 2));
		// z.x,z.y,w.x,w.y
		GGiVector4 vTemp2 = _mm_shuffle_ps(M.row[2], M.row[3], _MM_SHUFFLE(1, 0, 1, 0));
		// z.z,z.w,w.z,w.w
		GGiVector4 vTemp4 = _mm_shuffle_ps(M.row[2], M.row[3], _MM_SHUFFLE(3, 2, 3, 2));
		GGiFloat4x4 mResult;

		// x.x,y.x,z.x,w.x
		mResult.row[0] = _mm_shuffle_ps(vTemp1, vTemp2, _MM_SHUFFLE(2, 0, 2, 0));
		// x.y,y.y,z.y,w.y
		mResult.row[1] = _mm_shuffle_ps(vTemp1, vTemp2, _MM_SHUFFLE(3, 1, 3, 1));
		// x.z,y.z,z.z,w.z
		mResult.row[2] = _mm_shuffle_ps(vTemp3, vTemp4, _MM_SHUFFLE(2, 0, 2, 0));
		// x.w,y.w,z.w,w.w
		mResult.row[3] = _mm_shuffle_ps(vTemp3, vTemp4, _MM_SHUFFLE(3, 1, 3, 1));
		return mResult;
	}

	inline void _GGI_VECTOR_CALL Transpose()
	{
		// x.x,x.y,y.x,y.y
		GGiVector4 vTemp1 = _mm_shuffle_ps(row[0], row[1], _MM_SHUFFLE(1, 0, 1, 0));
		// x.z,x.w,y.z,y.w
		GGiVector4 vTemp3 = _mm_shuffle_ps(row[0], row[1], _MM_SHUFFLE(3, 2, 3, 2));
		// z.x,z.y,w.x,w.y
		GGiVector4 vTemp2 = _mm_shuffle_ps(row[2], row[3], _MM_SHUFFLE(1, 0, 1, 0));
		// z.z,z.w,w.z,w.w
		GGiVector4 vTemp4 = _mm_shuffle_ps(row[2], row[3], _MM_SHUFFLE(3, 2, 3, 2));

		// x.x,y.x,z.x,w.x
		row[0] = _mm_shuffle_ps(vTemp1, vTemp2, _MM_SHUFFLE(2, 0, 2, 0));
		// x.y,y.y,z.y,w.y
		row[1] = _mm_shuffle_ps(vTemp1, vTemp2, _MM_SHUFFLE(3, 1, 3, 1));
		// x.z,y.z,z.z,w.z
		row[2] = _mm_shuffle_ps(vTemp3, vTemp4, _MM_SHUFFLE(2, 0, 2, 0));
		// x.w,y.w,z.w,w.w
		row[3] = _mm_shuffle_ps(vTemp3, vTemp4, _MM_SHUFFLE(3, 1, 3, 1));
	}

	inline GGiVector4 _GGI_VECTOR_CALL GetDeterminant()
	{
		static const __m128 Sign = { 1.0f, -1.0f, 1.0f, -1.0f };

		GGiVector4 V0 = GGiVectorSwizzle<GGI_SWIZZLE_Y, GGI_SWIZZLE_X, GGI_SWIZZLE_X, GGI_SWIZZLE_X>(row[2]);
		GGiVector4 V1 = GGiVectorSwizzle<GGI_SWIZZLE_Z, GGI_SWIZZLE_Z, GGI_SWIZZLE_Y, GGI_SWIZZLE_Y>(row[3]);
		GGiVector4 V2 = GGiVectorSwizzle<GGI_SWIZZLE_Y, GGI_SWIZZLE_X, GGI_SWIZZLE_X, GGI_SWIZZLE_X>(row[2]);
		GGiVector4 V3 = GGiVectorSwizzle<GGI_SWIZZLE_W, GGI_SWIZZLE_W, GGI_SWIZZLE_W, GGI_SWIZZLE_Z>(row[3]);
		GGiVector4 V4 = GGiVectorSwizzle<GGI_SWIZZLE_Z, GGI_SWIZZLE_Z, GGI_SWIZZLE_Y, GGI_SWIZZLE_Y>(row[2]);
		GGiVector4 V5 = GGiVectorSwizzle<GGI_SWIZZLE_W, GGI_SWIZZLE_W, GGI_SWIZZLE_W, GGI_SWIZZLE_Z>(row[3]);

		GGiVector4 P0 = GGiVectorMultiply(V0, V1);
		GGiVector4 P1 = GGiVectorMultiply(V2, V3);
		GGiVector4 P2 = GGiVectorMultiply(V4, V5);

		V0 = GGiVectorSwizzle<GGI_SWIZZLE_Z, GGI_SWIZZLE_Z, GGI_SWIZZLE_Y, GGI_SWIZZLE_Y>(row[2]);
		V1 = GGiVectorSwizzle<GGI_SWIZZLE_Y, GGI_SWIZZLE_X, GGI_SWIZZLE_X, GGI_SWIZZLE_X>(row[3]);
		V2 = GGiVectorSwizzle<GGI_SWIZZLE_W, GGI_SWIZZLE_W, GGI_SWIZZLE_W, GGI_SWIZZLE_Z>(row[2]);
		V3 = GGiVectorSwizzle<GGI_SWIZZLE_Y, GGI_SWIZZLE_X, GGI_SWIZZLE_X, GGI_SWIZZLE_X>(row[3]);
		V4 = GGiVectorSwizzle<GGI_SWIZZLE_W, GGI_SWIZZLE_W, GGI_SWIZZLE_W, GGI_SWIZZLE_Z>(row[2]);
		V5 = GGiVectorSwizzle<GGI_SWIZZLE_Z, GGI_SWIZZLE_Z, GGI_SWIZZLE_Y, GGI_SWIZZLE_Y>(row[3]);

		P0 = GGiVectorNegativeMultiplySubtract(V0, V1, P0);
		P1 = GGiVectorNegativeMultiplySubtract(V2, V3, P1);
		P2 = GGiVectorNegativeMultiplySubtract(V4, V5, P2);

		V0 = GGiVectorSwizzle<GGI_SWIZZLE_W, GGI_SWIZZLE_W, GGI_SWIZZLE_W, GGI_SWIZZLE_Z>(row[1]);
		V1 = GGiVectorSwizzle<GGI_SWIZZLE_Z, GGI_SWIZZLE_Z, GGI_SWIZZLE_Y, GGI_SWIZZLE_Y>(row[1]);
		V2 = GGiVectorSwizzle<GGI_SWIZZLE_Y, GGI_SWIZZLE_X, GGI_SWIZZLE_X, GGI_SWIZZLE_X>(row[1]);

		GGiVector4 S = GGiVectorMultiply(row[0], Sign);
		GGiVector4 R = GGiVectorMultiply(V0, P0);
		R = GGiVectorNegativeMultiplySubtract(V1, P1, R);
		R = GGiVectorMultiplyAdd(V2, P2, R);

		return GGiVector4Dot(S, R);
	}

	static inline GGiVector4 _GGI_VECTOR_CALL GetDeterminant
	(
		GGiFloat4x4 M
	)
	{
		return M.GetDeterminant();
	}

	// Return the inverse and the determinant of a 4x4 matrix
	inline GGiFloat4x4 _GGI_VECTOR_CALL GetInverse()
	{
		auto det = GetDeterminant();
		GGiVector4* pDeterminant = &det;
		GGiFloat4x4 M = *this;

		GGiFloat4x4 MT = Transpose(M);
		GGiVector4 V00 = _GGI_SHUFFLE_PS(MT.row[2], _MM_SHUFFLE(1, 1, 0, 0));
		GGiVector4 V10 = _GGI_SHUFFLE_PS(MT.row[3], _MM_SHUFFLE(3, 2, 3, 2));
		GGiVector4 V01 = _GGI_SHUFFLE_PS(MT.row[0], _MM_SHUFFLE(1, 1, 0, 0));
		GGiVector4 V11 = _GGI_SHUFFLE_PS(MT.row[1], _MM_SHUFFLE(3, 2, 3, 2));
		GGiVector4 V02 = _mm_shuffle_ps(MT.row[2], MT.row[0], _MM_SHUFFLE(2, 0, 2, 0));
		GGiVector4 V12 = _mm_shuffle_ps(MT.row[3], MT.row[1], _MM_SHUFFLE(3, 1, 3, 1));

		GGiVector4 D0 = _mm_mul_ps(V00, V10);
		GGiVector4 D1 = _mm_mul_ps(V01, V11);
		GGiVector4 D2 = _mm_mul_ps(V02, V12);

		V00 = _GGI_SHUFFLE_PS(MT.row[2], _MM_SHUFFLE(3, 2, 3, 2));
		V10 = _GGI_SHUFFLE_PS(MT.row[3], _MM_SHUFFLE(1, 1, 0, 0));
		V01 = _GGI_SHUFFLE_PS(MT.row[0], _MM_SHUFFLE(3, 2, 3, 2));
		V11 = _GGI_SHUFFLE_PS(MT.row[1], _MM_SHUFFLE(1, 1, 0, 0));
		V02 = _mm_shuffle_ps(MT.row[2], MT.row[0], _MM_SHUFFLE(3, 1, 3, 1));
		V12 = _mm_shuffle_ps(MT.row[3], MT.row[1], _MM_SHUFFLE(2, 0, 2, 0));

		V00 = _mm_mul_ps(V00, V10);
		V01 = _mm_mul_ps(V01, V11);
		V02 = _mm_mul_ps(V02, V12);
		D0 = _mm_sub_ps(D0, V00);
		D1 = _mm_sub_ps(D1, V01);
		D2 = _mm_sub_ps(D2, V02);
		// V11 = D0Y,D0W,D2Y,D2Y
		V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 1, 3, 1));
		V00 = _GGI_SHUFFLE_PS(MT.row[1], _MM_SHUFFLE(1, 0, 2, 1));
		V10 = _mm_shuffle_ps(V11, D0, _MM_SHUFFLE(0, 3, 0, 2));
		V01 = _GGI_SHUFFLE_PS(MT.row[0], _MM_SHUFFLE(0, 1, 0, 2));
		V11 = _mm_shuffle_ps(V11, D0, _MM_SHUFFLE(2, 1, 2, 1));
		// V13 = D1Y,D1W,D2W,D2W
		GGiVector4 V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 3, 3, 1));
		V02 = _GGI_SHUFFLE_PS(MT.row[3], _MM_SHUFFLE(1, 0, 2, 1));
		V12 = _mm_shuffle_ps(V13, D1, _MM_SHUFFLE(0, 3, 0, 2));
		GGiVector4 V03 = _GGI_SHUFFLE_PS(MT.row[2], _MM_SHUFFLE(0, 1, 0, 2));
		V13 = _mm_shuffle_ps(V13, D1, _MM_SHUFFLE(2, 1, 2, 1));

		GGiVector4 C0 = _mm_mul_ps(V00, V10);
		GGiVector4 C2 = _mm_mul_ps(V01, V11);
		GGiVector4 C4 = _mm_mul_ps(V02, V12);
		GGiVector4 C6 = _mm_mul_ps(V03, V13);

		// V11 = D0X,D0Y,D2X,D2X
		V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(0, 0, 1, 0));
		V00 = _GGI_SHUFFLE_PS(MT.row[1], _MM_SHUFFLE(2, 1, 3, 2));
		V10 = _mm_shuffle_ps(D0, V11, _MM_SHUFFLE(2, 1, 0, 3));
		V01 = _GGI_SHUFFLE_PS(MT.row[0], _MM_SHUFFLE(1, 3, 2, 3));
		V11 = _mm_shuffle_ps(D0, V11, _MM_SHUFFLE(0, 2, 1, 2));
		// V13 = D1X,D1Y,D2Z,D2Z
		V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(2, 2, 1, 0));
		V02 = _GGI_SHUFFLE_PS(MT.row[3], _MM_SHUFFLE(2, 1, 3, 2));
		V12 = _mm_shuffle_ps(D1, V13, _MM_SHUFFLE(2, 1, 0, 3));
		V03 = _GGI_SHUFFLE_PS(MT.row[2], _MM_SHUFFLE(1, 3, 2, 3));
		V13 = _mm_shuffle_ps(D1, V13, _MM_SHUFFLE(0, 2, 1, 2));

		V00 = _mm_mul_ps(V00, V10);
		V01 = _mm_mul_ps(V01, V11);
		V02 = _mm_mul_ps(V02, V12);
		V03 = _mm_mul_ps(V03, V13);
		C0 = _mm_sub_ps(C0, V00);
		C2 = _mm_sub_ps(C2, V01);
		C4 = _mm_sub_ps(C4, V02);
		C6 = _mm_sub_ps(C6, V03);

		V00 = _GGI_SHUFFLE_PS(MT.row[1], _MM_SHUFFLE(0, 3, 0, 3));
		// V10 = D0Z,D0Z,D2X,D2Y
		V10 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 0, 2, 2));
		V10 = _GGI_SHUFFLE_PS(V10, _MM_SHUFFLE(0, 2, 3, 0));
		V01 = _GGI_SHUFFLE_PS(MT.row[0], _MM_SHUFFLE(2, 0, 3, 1));
		// V11 = D0X,D0W,D2X,D2Y
		V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 0, 3, 0));
		V11 = _GGI_SHUFFLE_PS(V11, _MM_SHUFFLE(2, 1, 0, 3));
		V02 = _GGI_SHUFFLE_PS(MT.row[3], _MM_SHUFFLE(0, 3, 0, 3));
		// V12 = D1Z,D1Z,D2Z,D2W
		V12 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 2, 2, 2));
		V12 = _GGI_SHUFFLE_PS(V12, _MM_SHUFFLE(0, 2, 3, 0));
		V03 = _GGI_SHUFFLE_PS(MT.row[2], _MM_SHUFFLE(2, 0, 3, 1));
		// V13 = D1X,D1W,D2Z,D2W
		V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 2, 3, 0));
		V13 = _GGI_SHUFFLE_PS(V13, _MM_SHUFFLE(2, 1, 0, 3));

		V00 = _mm_mul_ps(V00, V10);
		V01 = _mm_mul_ps(V01, V11);
		V02 = _mm_mul_ps(V02, V12);
		V03 = _mm_mul_ps(V03, V13);
		GGiVector4 C1 = _mm_sub_ps(C0, V00);
		C0 = _mm_add_ps(C0, V00);
		GGiVector4 C3 = _mm_add_ps(C2, V01);
		C2 = _mm_sub_ps(C2, V01);
		GGiVector4 C5 = _mm_sub_ps(C4, V02);
		C4 = _mm_add_ps(C4, V02);
		GGiVector4 C7 = _mm_add_ps(C6, V03);
		C6 = _mm_sub_ps(C6, V03);

		C0 = _mm_shuffle_ps(C0, C1, _MM_SHUFFLE(3, 1, 2, 0));
		C2 = _mm_shuffle_ps(C2, C3, _MM_SHUFFLE(3, 1, 2, 0));
		C4 = _mm_shuffle_ps(C4, C5, _MM_SHUFFLE(3, 1, 2, 0));
		C6 = _mm_shuffle_ps(C6, C7, _MM_SHUFFLE(3, 1, 2, 0));
		C0 = _GGI_SHUFFLE_PS(C0, _MM_SHUFFLE(3, 1, 2, 0));
		C2 = _GGI_SHUFFLE_PS(C2, _MM_SHUFFLE(3, 1, 2, 0));
		C4 = _GGI_SHUFFLE_PS(C4, _MM_SHUFFLE(3, 1, 2, 0));
		C6 = _GGI_SHUFFLE_PS(C6, _MM_SHUFFLE(3, 1, 2, 0));
		// Get the determinate
		GGiVector4 vTemp = GGiVector4Dot(C0, MT.row[0]);
		if (pDeterminant != nullptr)
			*pDeterminant = vTemp;
		vTemp = _mm_div_ps(g_GGiOne, vTemp);
		GGiFloat4x4 mResult;
		mResult.row[0] = _mm_mul_ps(C0, vTemp);
		mResult.row[1] = _mm_mul_ps(C2, vTemp);
		mResult.row[2] = _mm_mul_ps(C4, vTemp);
		mResult.row[3] = _mm_mul_ps(C6, vTemp);
		return mResult;
	}

	static inline GGiFloat4x4 _GGI_VECTOR_CALL GetInverse
	(
		GGiFloat4x4 M
	)
	{
		return M.GetInverse();
	}

};

