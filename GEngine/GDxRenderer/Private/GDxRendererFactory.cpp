#include "stdafx.h"
#include "GDxRendererFactory.h"
#include "GDxTexture.h"
#include "GDxTextureLoader.h"


GDxRendererFactory::GDxRendererFactory(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList,
	ID3D12CommandQueue* commandQueue)
{
	pDevice = device;
	pCommandList = commandList;
	pCommandQueue = commandQueue;
}


GDxRendererFactory::~GDxRendererFactory()
{
}

GRiTexture* GDxRendererFactory::CreateTexture()
{
	GRiTexture* ret = new GDxTexture();
	return ret;
}

GRiTextureLoader* GDxRendererFactory::CreateTextureLoader()
{
	GRiTextureLoader* ret = new GDxTextureLoader(pDevice, pCommandQueue);
	return ret;
}