#pragma once
#include "GGiPreInclude.h"
#include "GGiFloat4x4.h"
#include "GGiException.h"

class GGiFloat4
{

public:

	GGiFloat4();

	~GGiFloat4();

	virtual void AllowDynamicCast(){}

	virtual GGiFloat4& operator =(GGiFloat4& vec) 
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
		return *this;
	}

	virtual GGiFloat4& operator *(GGiFloat4& vec) 
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
		return *this; 
	}

	virtual GGiFloat4& operator +(GGiFloat4& vec)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
		return *this; 
	}

	virtual GGiFloat4& operator *(GGiFloat4x4& mat)
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
		return *this; 
	}

	virtual float GetX()
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
		return 0.0f;
	}

	virtual float GetY()
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
		return 0.0f;
	}

	virtual float GetZ()
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
		return 0.0f;
	}

	virtual float GetW()
	{
		ThrowGGiException("Instantiating GGiFloat4x4 will cause troubles.");
		return 0.0f;
	}

};

