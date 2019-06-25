#pragma once
#include "GGiPreInclude.h"
#include "GGiFloat4x4.h"
#include "GGiException.h"
#include "GGiMath.h"





class GGiFloat4
{


public:

	GGiFloat4();

	~GGiFloat4();

private:

	__m128 value;

public:

	/*
	virtual void AllowDynamicCast(){}

	virtual float GetElement(int i)
	{
		ThrowGGiException("Instantiating GGiFloat4 will cause troubles.");
		return -99999.0f;
	}

	virtual void SetElement(int i, float setValue)
	{
		ThrowGGiException("Instantiating GGiFloat4 will cause troubles.");
	}

	virtual GGiFloat4& operator =(GGiFloat4& vec) 
	{
		ThrowGGiException("Instantiating GGiFloat4 will cause troubles.");
		return *this;
	}

	virtual GGiFloat4& operator *(GGiFloat4& vec) 
	{
		ThrowGGiException("Instantiating GGiFloat4 will cause troubles.");
		return *this; 
	}

	virtual GGiFloat4& operator +(GGiFloat4& vec)
	{
		ThrowGGiException("Instantiating GGiFloat4 will cause troubles.");
		return *this; 
	}

	virtual GGiFloat4& operator *(GGiFloat4x4& mat)
	{
		ThrowGGiException("Instantiating GGiFloat4 will cause troubles.");
		return *this; 
	}

	virtual float GetX()
	{
		ThrowGGiException("Instantiating GGiFloat4 will cause troubles.");
		return 0.0f;
	}

	virtual float GetY()
	{
		ThrowGGiException("Instantiating GGiFloat4 will cause troubles.");
		return 0.0f;
	}

	virtual float GetZ()
	{
		ThrowGGiException("Instantiating GGiFloat4 will cause troubles.");
		return 0.0f;
	}

	virtual float GetW()
	{
		ThrowGGiException("Instantiating GGiFloat4 will cause troubles.");
		return 0.0f;
	}
	*/

	inline float GetElement(int i)
	{
		if (i < 0 || i > 3)
			ThrowGGiException("GGiFloat4 index error");

		return value.m128_f32[i];
	}

	inline void SetElement(int i, float setValue)
	{
		if (i < 0 || i > 3)
			ThrowGGiException("GGiFloat4 index error");

		value.m128_f32[i] = setValue;
	}

	inline GGiFloat4& operator =(GGiFloat4& vec)
	{
		value = vec.value;
		return *this;
	}

	inline GGiFloat4 operator *(GGiFloat4& vec)
	{
		GGiFloat4 ret;
		ret.value = _mm_mul_ps(value, vec.value);
		return ret;
	}

	inline GGiFloat4 operator +(GGiFloat4& vec)
	{
		GGiFloat4 ret;
		ret.value = _mm_add_ps(value, vec.value);
		return ret;
	}

	inline GGiFloat4 _GGI_VECTOR_CALL operator *(GGiFloat4x4& mat)
	{
		// Splat x,y,z and w
		GGiVector4 vTempX = _GGI_SHUFFLE_PS(value, _MM_SHUFFLE(0, 0, 0, 0));
		GGiVector4 vTempY = _GGI_SHUFFLE_PS(value, _MM_SHUFFLE(1, 1, 1, 1));
		GGiVector4 vTempZ = _GGI_SHUFFLE_PS(value, _MM_SHUFFLE(2, 2, 2, 2));
		GGiVector4 vTempW = _GGI_SHUFFLE_PS(value, _MM_SHUFFLE(3, 3, 3, 3));
		// Mul by the matrix
		vTempX = _mm_mul_ps(vTempX, mat.row[0]);
		vTempY = _mm_mul_ps(vTempY, mat.row[1]);
		vTempZ = _mm_mul_ps(vTempZ, mat.row[2]);
		vTempW = _mm_mul_ps(vTempW, mat.row[3]);
		// Add them all together
		vTempX = _mm_add_ps(vTempX, vTempY);
		vTempZ = _mm_add_ps(vTempZ, vTempW);
		vTempX = _mm_add_ps(vTempX, vTempZ);
		// Return value
		GGiFloat4 ret;
		ret.value = vTempX;
		return ret;
	}

	inline float GetX()
	{
		return value.m128_f32[0];
	}

	inline float GetY()
	{
		return value.m128_f32[1];
	}

	inline float GetZ()
	{
		return value.m128_f32[2];
	}

	inline float GetW()
	{
		return value.m128_f32[3];
	}

};


