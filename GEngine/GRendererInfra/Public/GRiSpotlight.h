#pragma once
#include "GRiPreInclude.h"


class GRiSpotlight
{
public:
	GRiSpotlight();
	~GRiSpotlight();

	float Color[4];
	float Direction[4];
	float Position[3];
	float Range;
	float SpotlightAngle;
};

