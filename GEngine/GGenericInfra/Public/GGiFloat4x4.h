#pragma once
#include "GGiPreInclude.h"
#include "GGiException.h"






static const __m128 GGiIdentityRow0 = { 1.0f, 0.0f, 0.0f, 0.0f };
static const __m128 GGiIdentityRow1 = { 0.0f, 1.0f, 0.0f, 0.0f };
static const __m128 GGiIdentityRow2 = { 0.0f, 0.0f, 1.0f, 0.0f };
static const __m128 GGiIdentityRow3 = { 0.0f, 0.0f, 0.0f, 1.0f };


class GGiFloat4x4
{

public:

	GGiFloat4x4();

	~GGiFloat4x4();

	/*
	virtual void AllowDynamicCast(){}

	virtual float GetElement(int i, int j)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
	}

	virtual void SetElement(int i, int j, float value)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
	}

	virtual GGiFloat4x4& operator =(GGiFloat4x4& mat) 
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
		return *this; 
	}

	virtual GGiFloat4x4& operator *(GGiFloat4x4& mat)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
		return *this;
	}

	virtual GGiFloat4x4& operator +(GGiFloat4x4& mat)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
		return *this; 
	}

	virtual void SetByTranslation(float x, float y, float z)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
	}

	virtual void SetByRotationPitchYawRoll(float pitch, float yaw, float roll)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
	}

	virtual void SetByScale(float x, float y, float z)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
	}
	
	virtual void SetByPerspectiveFovLH(float fovAngleY, float aspectRatio, float nearZ, float farZ)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
	}

	virtual void SetByRotationAxis(float axisX, float axisY, float axisZ, float angle)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
	}

	virtual void SetByRotationY(float angle)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
	}

	virtual std::vector<float> TransformNormal(std::vector<float> vec)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
		std::vector<float> ret = {};
		return ret;
	}

	virtual void Transpose()
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
	}

	virtual GGiFloat4x4* GetInverse()
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
		return nullptr;
	}
	*/

	inline float GetElement(int i, int j)
	{
		if (i < 0 || i>3 || j < 0 || j>3)
			ThrowGGiException("GGiFloat4x4 index error");

		return row[i].m128_f32[j];
	}

	inline void SetElement(int i, int j, float value)
	{
		if (i < 0 || i>3 || j < 0 || j>3)
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

	virtual inline GGiFloat4x4& operator +(GGiFloat4x4& mat)
	{
		GGiFloat4x4 mResult;

		mResult.row[0] = _mm_add_ps(row[0], mat.row[0]);
		mResult.row[1] = _mm_add_ps(row[1], mat.row[1]);
		mResult.row[2] = _mm_add_ps(row[2], mat.row[2]);
		mResult.row[3] = _mm_add_ps(row[3], mat.row[3]);

		return mResult;
	}

	inline void SetByTranslation(float x, float y, float z)
	{
		row[0] = GGiIdentityRow0;
		row[1] = GGiIdentityRow1;
		row[2] = GGiIdentityRow2;
		row[3] = _mm_set_ps(1.f, z, y, x); 
	}

	static inline GGiFloat4x4 GetMatrixByTranslation(float x, float y, float z)
	{
		GGiFloat4x4 M;
		M.row[0] = GGiIdentityRow0;
		M.row[1] = GGiIdentityRow1;
		M.row[2] = GGiIdentityRow2;
		M.row[3] = _mm_set_ps(1.f, z, y, x);
		return M;
	}

	virtual void SetByRotationPitchYawRoll(float pitch, float yaw, float roll)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
	}

	virtual void SetByScale(float x, float y, float z)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
	}

	virtual void SetByPerspectiveFovLH(float fovAngleY, float aspectRatio, float nearZ, float farZ)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
	}

	virtual void SetByRotationAxis(float axisX, float axisY, float axisZ, float angle)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
	}

	virtual void SetByRotationY(float angle)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
	}

	virtual std::vector<float> TransformNormal(std::vector<float> vec)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
		std::vector<float> ret = {};
		return ret;
	}

	virtual void Transpose()
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
	}

	virtual GGiFloat4x4* GetInverse()
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
		return nullptr;
	}

private:

	__m128 row[4];

};

