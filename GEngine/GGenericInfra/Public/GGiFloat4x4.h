#pragma once
#include "GGiPreInclude.h"

class GGiFloat4x4
{
public:
	GGiFloat4x4();
	~GGiFloat4x4();

	virtual void AllowDynamicCast(){}

	virtual GGiFloat4x4& operator =(GGiFloat4x4& mat) { return *this; }
	virtual GGiFloat4x4& operator *(GGiFloat4x4& mat) { return *this; }
	virtual GGiFloat4x4& operator +(GGiFloat4x4& mat) { return *this; }

};

