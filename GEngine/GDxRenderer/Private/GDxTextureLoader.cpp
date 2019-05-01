#include "stdafx.h"
#include "GDxTextureLoader.h"

#include "GDxTexture.h"

GDxTextureLoader::~GDxTextureLoader()
{
}

GDxTextureLoader::GDxTextureLoader(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12CommandQueue* cmdQueue)
{
	pDevice = device;
	pCommandList = cmdList;
	pCommandQueue = cmdQueue;
}

GRiTexture* GDxTextureLoader::LoadTexture(std::wstring workdir, std::wstring path, bool bSrgb)
{
	std::wstring lpath = path;
	std::transform(lpath.begin(), lpath.end(), lpath.begin(), ::tolower);
	std::wstring ext = GetExtension(lpath);
	std::wstring filename = workdir + path;
	std::wstring name = GetFileName(path);

	GDxTexture* tex = new GDxTexture();
	tex->UniqueFileName = path;
	tex->Name = name;
	tex->bSrgb = bSrgb;

	if (ext == L"dds")
	{
		DirectX::ResourceUploadBatch resourceUpload(pDevice);
		resourceUpload.Begin();

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
	else if (ext == L"tga")
	{
		DirectX::ScratchImage image;
		auto hr = DirectX::LoadFromTGAFile(filename.c_str(), nullptr, image);

		if (SUCCEEDED(hr))
		{
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> uploadCmdListAlloc;
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> uploadCommandList;
			ThrowIfFailed(pDevice->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(uploadCmdListAlloc.GetAddressOf())));

			ThrowIfFailed(pDevice->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				uploadCmdListAlloc.Get(), // Associated command allocator
				nullptr,                   // Initial PipelineStateObject
				IID_PPV_ARGS(&uploadCommandList)));

			//uploadCommandList->Close();
			//ThrowIfFailed(uploadCmdListAlloc->Reset());
			//ThrowIfFailed(uploadCommandList->Reset(uploadCmdListAlloc.Get(), nullptr));

			//
			// Create texutre.
			//
			auto hr = CreateTextureEx(pDevice, image.GetMetadata(), D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE, bSrgb, &tex->Resource);
			if (FAILED(hr))
				ThrowDxException(L"Failed to create texture.");

			std::vector<D3D12_SUBRESOURCE_DATA> subresources;
			hr = PrepareUpload(pDevice, image.GetImages(), image.GetImageCount(), image.GetMetadata(), subresources);
			if (FAILED(hr))
				ThrowDxException(L"Failed to prepare upload.");

			// upload is implemented by application developer. Here's one solution using <d3dx12.h>
			//const UINT64 uploadBufferSize = GetRequiredIntermediateSize(pResource, 0, static_cast<unsigned int>(subresources.size()));
			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(tex->Resource.Get(), 0, static_cast<unsigned int>(subresources.size()));

			//Microsoft::WRL::ComPtr<ID3D12Resource> textureUploadHeap;
			hr = pDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(tex->UploadHeap.GetAddressOf()));
			if (FAILED(hr))
				ThrowDxException(L"Failed to create resource.");

			UpdateSubresources(uploadCommandList.Get(),
				tex->Resource.Get(), tex->UploadHeap.Get(),
				0, 0, static_cast<unsigned int>(subresources.size()),
				subresources.data());

			// Done recording commands.
			ThrowIfFailed(uploadCommandList->Close());

			// Add the command list to the queue for execution.
			ID3D12CommandList* cmdsLists[] = { uploadCommandList.Get() };
			pCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
		}
	}
	else if (ext == L"jpg")
	{
		DirectX::ResourceUploadBatch resourceUpload(pDevice);
		resourceUpload.Begin();

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

/*
void GDxTextureLoader::LoadTexture(GRiTexture* pTexture, std::wstring workdir, std::wstring path, bool bSrgb)
{
	GDxTexture* dxTexture = dynamic_cast<GDxTexture*>(pTexture);
	if (dxTexture == nullptr)
		ThrowGGiException("Cast failed from GRiTexture* to GDxTexture*.");

	std::wstring lpath = path;
	std::transform(lpath.begin(), lpath.end(), lpath.begin(), ::tolower);
	std::wstring ext = GetExtension(lpath);
	std::wstring filename = workdir + path;

	if (ext == L"dds")
	{
		DirectX::ResourceUploadBatch resourceUpload(pDevice);
		resourceUpload.Begin();

		unsigned int srgbFlag;
		if (bSrgb)
		{
			srgbFlag = DirectX::WIC_LOADER_FORCE_SRGB;
		}
		else
		{
			srgbFlag = DirectX::WIC_LOADER_IGNORE_SRGB;
		}
		ThrowIfFailed(DirectX::CreateDDSTextureFromFileEx(pDevice, resourceUpload, filename.c_str(), 0u, D3D12_RESOURCE_FLAG_NONE, srgbFlag, dxTexture->Resource.ReleaseAndGetAddressOf()));

		auto uploadResourcesFinished = resourceUpload.End(pCommandQueue);
		uploadResourcesFinished.wait();
	}
	else if (ext == L"png")
	{
		DirectX::ResourceUploadBatch resourceUpload(pDevice);
		resourceUpload.Begin();

		unsigned int srgbFlag;
		if (bSrgb)
		{
			srgbFlag = DirectX::WIC_LOADER_FORCE_SRGB;
		}
		else
		{
			srgbFlag = DirectX::WIC_LOADER_IGNORE_SRGB;
		}
		ThrowIfFailed(CreateWICTextureFromFileEx(pDevice, resourceUpload, filename.c_str(), 0u, D3D12_RESOURCE_FLAG_NONE, srgbFlag, dxTexture->Resource.ReleaseAndGetAddressOf()));

		auto uploadResourcesFinished = resourceUpload.End(pCommandQueue);
		uploadResourcesFinished.wait();
	}
}
*/


