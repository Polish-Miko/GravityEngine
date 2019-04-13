#pragma once

//#include "GUtilInclude.h"
#include "GDxPreInclude.h"

class GSpotlight
{
public:
	GSpotlight(){}
	~GSpotlight(){}

	DirectX::XMFLOAT4 Color;
	DirectX::XMFLOAT4 Direction;
	DirectX::XMFLOAT3 Position;
	float Range;
	float SpotlightAngle;
};

