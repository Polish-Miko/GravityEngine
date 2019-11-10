#pragma once
#include "GDxPreInclude.h"

class GDxCascadedShadowMap
{

public:

	GDxCascadedShadowMap(ID3D12Device* pDevice, UINT res);

	~GDxCascadedShadowMap();

	ID3D12Resource* GetShadowmapResource();

private:

	UINT64 Resolution = 1024;

	Microsoft::WRL::ComPtr<ID3D12Resource> mShadowmapResource;

};

