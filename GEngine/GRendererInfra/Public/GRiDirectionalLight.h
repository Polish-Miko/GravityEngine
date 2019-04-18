#pragma once
#include "GRiPreInclude.h"


class GRiDirectionalLight
{
public:
	GRiDirectionalLight();
	~GRiDirectionalLight();

	float AmbientColor[4];
	float DiffuseColor[4];
	float Direction[3];
	float Intensity;
};

