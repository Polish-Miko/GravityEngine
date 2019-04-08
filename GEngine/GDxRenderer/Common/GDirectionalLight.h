#pragma once

#include "GUtilInclude.h"

class GDirectionalLight
{
public:
	GDirectionalLight(){}
	~GDirectionalLight(){}

	DirectX::XMFLOAT4 AmbientColor;
	DirectX::XMFLOAT4 DiffuseColor;
	DirectX::XMFLOAT3 Direction;
	float Intensity;
};

