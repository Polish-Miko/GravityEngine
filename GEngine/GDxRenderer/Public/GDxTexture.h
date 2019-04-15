#pragma once
#include "GDxPreInclude.h"
#include "GRiInclude.h"

class GDxTexture : public GRiTexture
{

public:

	GDxTexture();
	~GDxTexture();

	int texIndex;

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;

	virtual void AllowDynamicCast() override;
};

