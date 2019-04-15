#pragma once
#include "GGiPreInclude.h"

class GGiFloat4
{
public:
	GGiFloat4();
	~GGiFloat4();

	virtual void AllowDynamicCast(){}

	virtual GGiFloat4& operator =(GGiFloat4& vec) = 0;
	virtual GGiFloat4& operator *(GGiFloat4& vec) = 0;
	virtual GGiFloat4& operator +(GGiFloat4& vec) = 0;
};

