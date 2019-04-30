#pragma once
#include "GDxPreInclude.h"


class GDxSceneObject : public GRiSceneObject
{

public:

	virtual GGiFloat4x4* GetTransform() override;

	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology();

	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topo);

private:

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

};

