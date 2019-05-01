#pragma once
#include "GDxPreInclude.h"
#include "GRiInclude.h"

class GDxTexture : public GRiTexture
{

public:

	GDxTexture();
	~GDxTexture();

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;

	//std::vector<D3D12_SUBRESOURCE_DATA> Subresources;

	virtual void AllowDynamicCast() override;
};

