#pragma once
#include "GRiPreInclude.h"


class GRiVertex
{
public:
	GRiVertex();
	~GRiVertex();

	GRiVertex(
		float px, float py, float pz,
		float nx, float ny, float nz,
		float tx, float ty, float tz,
		float u, float v)
	{
		Position[0] = px;
		Position[1] = py;
		Position[2] = pz;
		Normal[0] = nx;
		Normal[1] = ny;
		Normal[2] = nz;
		TangentU[0] = tx;
		TangentU[1] = ty;
		TangentU[2] = tz;
		UV[0] = u;
		UV[1] = v;
	}

	float Position[3];
	float UV[2];
	float Normal[3];
	float TangentU[3]; 
};

