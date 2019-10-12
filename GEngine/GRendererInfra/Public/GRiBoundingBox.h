#pragma once
#include "GRiPreInclude.h"



class GRiBoundingBox
{
	/*
public:

	GRiBoundingBox();
	~GRiBoundingBox();
	*/

public:

	float Center[3];

	float Extents[3];

	static GRiBoundingBox Union(GRiBoundingBox a, GRiBoundingBox b);

	float SurfaceArea() const 
	{
		return (2 * 4 * (Extents[0] * Extents[1] + Extents[0] * Extents[2] + Extents[1] * Extents[2]));
	}

	int MaximumExtent() const
	{
		if (Extents[0] >= Extents[1] && Extents[0] >= Extents[2])
			return 0;
		else if (Extents[1] >= Extents[2])
			return 1;
		else
			return 2;
	}

	float BoundMin(int axis) const
	{
		return (Center[axis] - Extents[axis]);
	}

	float BoundMax(int axis) const
	{
		return (Center[axis] + Extents[axis]);
	}

};

