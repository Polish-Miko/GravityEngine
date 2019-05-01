#pragma once
#include "GDxPreInclude.h"

class GDxTextureLoader : public GRiTextureLoader
{

public:
	GDxTextureLoader() = delete;
	~GDxTextureLoader();

	GDxTextureLoader(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12CommandQueue* cmdQueue);

	virtual GRiTexture* LoadTexture(std::wstring workdir, std::wstring path, bool bSrgb) override;

	//virtual void LoadTexture(GRiTexture* pTexture, std::wstring workdir, std::wstring path, bool bSrgb) override;

private:

	ID3D12Device* pDevice;

	ID3D12GraphicsCommandList* pCommandList;

	ID3D12CommandQueue* pCommandQueue;

};

