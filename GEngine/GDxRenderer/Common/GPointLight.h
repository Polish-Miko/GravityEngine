#pragma once

#include "GUtilInclude.h"

class GPointLight
{
public:
	GPointLight(){}
	~GPointLight(){}

	DirectX::XMFLOAT4 Color;
	DirectX::XMFLOAT3 Position;
	float Range;
	float Intensity;
	DirectX::XMFLOAT3 Padding;
};

