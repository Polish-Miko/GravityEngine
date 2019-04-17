#include "stdafx.h"
#include "GDxRendererFactory.h"
#include "GDxTexture.h"
#include "GDxTextureLoader.h"
#include "GDxFloat4.h"
#include "GDxFloat4x4.h"
#include "GDxMaterial.h"


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

GRiMaterial* GDxRendererFactory::CreateMaterial()
{
	GRiMaterial* ret = new GDxMaterial();
	return ret;
}

GGiFloat4* GDxRendererFactory::CreateFloat4()
{
	GGiFloat4* ret = GDxFloat4::ZeroVector();
	return ret;
}

GGiFloat4* GDxRendererFactory::CreateFloat4(float x, float y, float z, float w)
{
	GGiFloat4* ret = new GDxFloat4(x, y, z, w);
	return ret;
}

GGiFloat4x4* GDxRendererFactory::CreateFloat4x4()
{
	GGiFloat4x4* ret = GDxFloat4x4::Identity4x4();
	return ret;
}