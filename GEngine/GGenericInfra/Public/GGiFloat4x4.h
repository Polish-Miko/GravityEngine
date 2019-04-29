#pragma once
#include "GGiPreInclude.h"
#include "GGiException.h"

class GGiFloat4x4
{
public:
	GGiFloat4x4();
	~GGiFloat4x4();

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

};

