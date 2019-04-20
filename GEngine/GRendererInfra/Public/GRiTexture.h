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

	int texIndex = -1;

	virtual void AllowDynamicCast() = 0;
};

