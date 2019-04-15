#pragma once
#include "GRiPreInclude.h"

class GRiTexture
{

public:

	GRiTexture();
	~GRiTexture();

	std::wstring UniqueFileName;
	std::wstring Name;

	bool bSrgb;

	virtual void AllowDynamicCast() = 0;
};

