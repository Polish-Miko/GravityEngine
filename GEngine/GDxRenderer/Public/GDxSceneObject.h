#pragma once
#include "GDxPreInclude.h"


class GDxSceneObject : public GRiSceneObject
{

public:

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	virtual GGiFloat4x4* GetTransform() override;

};

