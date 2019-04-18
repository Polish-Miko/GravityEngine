#pragma once
#include "GRiPreInclude.h"

class GRiPointLight
{
public:
	GRiPointLight();
	~GRiPointLight();

	float Color[4];
	float Position[3];
	float Range;
	float Intensity;
	float Padding[3];
};

