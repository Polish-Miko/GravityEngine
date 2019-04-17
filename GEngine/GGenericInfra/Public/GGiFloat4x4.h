#pragma once
#include "GGiPreInclude.h"
#include "GGiException.h"

class GGiFloat4x4
{
public:
	GGiFloat4x4();
	~GGiFloat4x4();

	virtual void AllowDynamicCast(){}

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

};

