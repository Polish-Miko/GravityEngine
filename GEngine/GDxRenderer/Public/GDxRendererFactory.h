#pragma once
#include "GDxPreInclude.h"
#include "GRiInclude.h"

class GDxRendererFactory : public GRiRendererFactory
{

public:
	GDxRendererFactory() = delete;
	~GDxRendererFactory();

	GDxRendererFactory(ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		ID3D12CommandQueue* commandQueue);

	virtual GRiTexture* CreateTexture() override;

	virtual GRiTextureLoader* CreateTextureLoader() override;

	virtual GRiMaterial* CreateMaterial() override;

private:

	ID3D12Device* pDevice;
	ID3D12GraphicsCommandList* pCommandList;
	ID3D12CommandQueue* pCommandQueue;

};

