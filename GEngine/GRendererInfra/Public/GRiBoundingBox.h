#pragma once
#include "GRiPreInclude.h"
#include "GRiRay.h"



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

	bool Intersect(const GRiRay &ray, float *hitt0, float *hitt1) const
	{
		float t0 = 0, t1 = ray.tMax;
		for (int i = 0; i < 3; ++i)
		{
			// Update interval for _i_th bounding box slab
			float invRayDir = 1 / ray.Direction[i];
			float tNear = (Center[i] - Extents[i] - ray.Origin[i]) * invRayDir;
			float tFar = (Center[i] + Extents[i] - ray.Origin[i]) * invRayDir;

			// Update parametric interval from slab intersection $t$ values
			if (tNear > tFar)
				std::swap(tNear, tFar);

			// Update _tFar_ to ensure robust ray--bounds intersection
			tFar *= 1 + 2 * GGiEngineUtil::gamma(3);
			t0 = tNear > t0 ? tNear : t0;
			t1 = tFar < t1 ? tFar : t1;
			if (t0 > t1)
				return false;
		}
		if (hitt0)
			*hitt0 = t0;
		if (hitt1)
			*hitt1 = t1;
		return true;
	}

};

