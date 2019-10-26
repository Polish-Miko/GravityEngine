#pragma once
#include "GRiPreInclude.h"


class GRiRay
{

public:

	GRiRay();
	~GRiRay();


	float Origin[3];
	float Direction[3];
	mutable float tMax = 999999999;
	mutable bool bBackface = false;

};

