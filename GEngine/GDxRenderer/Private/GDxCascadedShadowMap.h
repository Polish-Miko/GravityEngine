#pragma once
#include "GDxPreInclude.h"

class GDxCascadedShadowMap
{

public:

	GDxCascadedShadowMap(ID3D12Device* pDevice, UINT res);

	~GDxCascadedShadowMap();

	ID3D12Resource* GetShadowmapResource();

	ID3D12Resource* GetXPrefilteredResource();

	ID3D12Resource* GetYPrefilteredResource();

private:

	UINT64 Resolution = 2048;

	Microsoft::WRL::ComPtr<ID3D12Resource> mShadowmapResource;

	Microsoft::WRL::ComPtr<ID3D12Resource> mXPrefilteredShadowmapResource;

	Microsoft::WRL::ComPtr<ID3D12Resource> mYPrefilteredShadowmapResource;

};

