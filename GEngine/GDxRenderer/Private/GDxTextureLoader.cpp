#include "stdafx.h"
#include "GDxTextureLoader.h"

#include "GDxTexture.h"

GDxTextureLoader::~GDxTextureLoader()
{
}

GDxTextureLoader::GDxTextureLoader(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
	pDevice = device;
	pCommandQueue = cmdQueue;
}

GRiTexture* GDxTextureLoader::LoadTexture(std::wstring workdir, std::wstring path, bool bSrgb)
{
	std::wstring lpath = path;
	std::transform(lpath.begin(), lpath.end(), lpath.begin(), ::tolower);
	std::wstring ext = GetExtension(lpath);
	std::wstring name = GetFileName(path);
	std::wstring filename = workdir + path;

	GDxTexture* tex = new GDxTexture();
	tex->UniqueFileName = path;
	tex->Name = name;
	//tex->texIndex = texIndex;
	tex->bSrgb = bSrgb;

	if (ext == L"dds")
	{
		DirectX::ResourceUploadBatch resourceUpload(pDevice);
		resourceUpload.Begin();
		//ThrowIfFailed(DirectX::CreateDDSTextureFromFile(pDevice, resourceUpload, filename.c_str(), tex->Resource.ReleaseAndGetAddressOf()));

		unsigned int srgbFlag;
		if (bSrgb)
		{
			srgbFlag = DirectX::WIC_LOADER_FORCE_SRGB;
		}
		else
		{
			srgbFlag = DirectX::WIC_LOADER_IGNORE_SRGB;
		}
		ThrowIfFailed(DirectX::CreateDDSTextureFromFileEx(pDevice, resourceUpload, filename.c_str(), 0u, D3D12_RESOURCE_FLAG_NONE, srgbFlag, tex->Resource.ReleaseAndGetAddressOf()));

		auto uploadResourcesFinished = resourceUpload.End(pCommandQueue);
		uploadResourcesFinished.wait();
	}
	else if (ext == L"png")
	{
		DirectX::ResourceUploadBatch resourceUpload(pDevice);
		resourceUpload.Begin();
		//ThrowIfFailed(CreateWICTextureFromFile(pDevice, resourceUpload, filename.c_str(), tex->Resource.ReleaseAndGetAddressOf()));

		unsigned int srgbFlag;
		if (bSrgb)
		{
			srgbFlag = DirectX::WIC_LOADER_FORCE_SRGB;
		}
		else
		{
			srgbFlag = DirectX::WIC_LOADER_IGNORE_SRGB;
		}
		ThrowIfFailed(CreateWICTextureFromFileEx(pDevice, resourceUpload, filename.c_str(), 0u, D3D12_RESOURCE_FLAG_NONE, srgbFlag, tex->Resource.ReleaseAndGetAddressOf()));

		auto uploadResourcesFinished = resourceUpload.End(pCommandQueue);
		uploadResourcesFinished.wait();
	}
	return tex;
}