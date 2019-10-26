#include "stdafx.h"
#include "GRiBoundingBox.h"

/*
GRiBoundingBox::GRiBoundingBox()
{
}


GRiBoundingBox::~GRiBoundingBox()
{
}
*/


GRiBoundingBox GRiBoundingBox::Union(GRiBoundingBox a, GRiBoundingBox b)
{
	GRiBoundingBox ret;
	float bMin[3], bMax[3];

	for (auto i = 0u; i < 3; i++)
	{
		bMin[i] = min(a.Center[i] - a.Extents[i], b.Center[i] - b.Extents[i]);
		bMax[i] = max(a.Center[i] + a.Extents[i], b.Center[i] + b.Extents[i]);
	}

	ret.Center[0] = (bMax[0] + bMin[0]) / 2;
	ret.Center[1] = (bMax[1] + bMin[1]) / 2;
	ret.Center[2] = (bMax[2] + bMin[2]) / 2;
	ret.Extents[0] = (bMax[0] - bMin[0]) / 2;
	ret.Extents[1] = (bMax[1] - bMin[1]) / 2;
	ret.Extents[2] = (bMax[2] - bMin[2]) / 2;

	return ret;
}

