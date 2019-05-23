#include "stdafx.h"
#include "GDxUav.h"
#include "GDxUtil.h"


/*
GDxUav::GDxUav()
{
}


GDxUav::~GDxUav()
{
}
*/

GDxUav::GDxUav(ID3D12Device* device, UINT ClientWidth, UINT ClientHeight,
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrv,
	GDxUavProperties uavProperties,
	bool ScaledByViewport,
	bool CreateRtv,
	bool isTexture,
	UINT64 elementSizeInByte,
	UINT64 numElement
	)
{
	md3dDevice = device;
	mClientWidth = ClientWidth;
	mClientHeight = ClientHeight;
	mProperties = uavProperties;
	SetDescriptorSize();
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHeap = cpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHeap = gpuSrv;
	mCpuSrv = cpuHeap;
	mGpuSrv = gpuHeap;
	cpuHeap.Offset(SrvUavDescriptorSize);
	gpuHeap.Offset(SrvUavDescriptorSize);
	mCpuUav = cpuHeap;
	mGpuUav = gpuHeap;
	bScaledByViewport = ScaledByViewport;
	bCreateRtv = CreateRtv;
	bIsTexture = isTexture;
	elementByteSize = elementSizeInByte;
	//elementByteSize = GDxUtil::CalcConstantBufferByteSize(elementSizeInByte);
	elementNum = numElement;

	if (bScaledByViewport && bIsTexture)
	{
		SetViewportAndScissorRect((UINT)(ClientWidth * mProperties.mWidthPercentage), (UINT)(ClientHeight * mProperties.mHeightPercentage));
	}
	else
	{
		SetViewportAndScissorRect(ClientWidth, ClientHeight);
	}

	if (bCreateRtv)
	{
		GRtvProperties rtvProperties;
		rtvProperties.mClearColor[0] = mProperties.mClearColor[0];
		rtvProperties.mClearColor[1] = mProperties.mClearColor[1];
		rtvProperties.mClearColor[2] = mProperties.mClearColor[2];
		rtvProperties.mClearColor[3] = mProperties.mClearColor[3];
		rtvProperties.mHeightPercentage = mProperties.mHeightPercentage;
		rtvProperties.mWidthPercentage = mProperties.mWidthPercentage;
		rtvProperties.mRtvFormat = mProperties.mUavFormat;
		mRtv = std::make_shared<GDxRtv>(rtvProperties, ClientWidth, ClientHeight);

		ThrowIfFailed(mRtvHeap.Create(md3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, (UINT)1));
	}

	BuildResources();
	BuildDescriptors();
}

void GDxUav::BuildDescriptors()
{
	if (bIsTexture)
	{
		auto hr = md3dDevice->GetDeviceRemovedReason();
		hr = hr;

		// Create UAV.
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

		uavDesc.Format = mProperties.mUavFormat;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;

		md3dDevice->CreateUnorderedAccessView(mResource.Get(), nullptr, &uavDesc, mCpuUav);

		hr = md3dDevice->GetDeviceRemovedReason();
		hr = hr;

		// Create SRV.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;

		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = mProperties.mUavFormat;
		srvDesc.Texture2D.MipLevels = 1;

		md3dDevice->CreateShaderResourceView(mResource.Get(), &srvDesc, mCpuSrv);

		hr = md3dDevice->GetDeviceRemovedReason();
		hr = hr;

		// Create RTV
		if (bCreateRtv)
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
			ZeroMemory(&rtvDesc, sizeof(rtvDesc));
			rtvDesc.Texture2D.MipSlice = 0;
			rtvDesc.Texture2D.PlaneSlice = 0;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Format = mRtv->mProperties.mRtvFormat;

			md3dDevice->CreateRenderTargetView(mResource.Get(), &rtvDesc, mRtvHeap.handleCPU(0));
			mRtv->mRtvCpu = mRtvHeap.handleCPU(0);

			hr = md3dDevice->GetDeviceRemovedReason();
			hr = hr;
		}
	}
	else
	{// Create UAV.
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.NumElements = elementNum;
		uavDesc.Buffer.StructureByteStride = elementByteSize;
		//uavDesc.Buffer.NumElements = 1;
		//uavDesc.Buffer.StructureByteStride = 1024;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		md3dDevice->CreateUnorderedAccessView(mResource.Get(), nullptr, &uavDesc, mCpuUav);

		// Create SRV.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;

		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = elementNum;
		srvDesc.Buffer.StructureByteStride = elementByteSize;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		md3dDevice->CreateShaderResourceView(mResource.Get(), &srvDesc, mCpuSrv);
	}
}

void GDxUav::BuildResources()
{
	if (bIsTexture)
	{
		D3D12_RESOURCE_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Alignment = 0;
		texDesc.DepthOrArraySize = 1;
		texDesc.MipLevels = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		// Free the old resources if they exist.
		if (mResource != nullptr)
			mResource.Reset();

		texDesc.Width = (UINT)(mProperties.mWidthPercentage * mClientWidth);
		texDesc.Height = (UINT)(mProperties.mHeightPercentage * mClientHeight);
		texDesc.Format = mProperties.mUavFormat;
		float normalClearColor[4];
		normalClearColor[0] = mProperties.mClearColor[0];
		normalClearColor[1] = mProperties.mClearColor[1];
		normalClearColor[2] = mProperties.mClearColor[2];
		normalClearColor[3] = mProperties.mClearColor[3];
		CD3DX12_CLEAR_VALUE optClear(mProperties.mUavFormat, normalClearColor);

		ThrowIfFailed(md3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			&optClear,
			IID_PPV_ARGS(&mResource)));
	}
	else
	{
		// Free the old resources if they exist.
		if (mResource != nullptr)
			mResource.Reset();

		ThrowIfFailed(md3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(elementByteSize * elementNum, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			//&CD3DX12_RESOURCE_DESC::Buffer(1024, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mResource)));
	}

}

void GDxUav::OnResize(UINT newWidth, UINT newHeight)
{
	if (mClientWidth != newWidth || mClientHeight != newHeight && !bIsTexture)
	{
		if (bScaledByViewport)
		{
			mClientWidth = newWidth;
			mClientHeight = newHeight;
			SetViewportAndScissorRect((UINT)(mClientWidth * mProperties.mWidthPercentage), (UINT)(mClientHeight * mProperties.mHeightPercentage));
			if (bCreateRtv)
			{
				mRtv->OnResize(newWidth, newHeight);
			}
		}

		BuildResources();
		BuildDescriptors();
	}
}

void GDxUav::OnBufferResize(UINT newElementNum)
{
	if (!bIsTexture)
	{
		elementNum = newElementNum;

		BuildResources();
		BuildDescriptors();
	}
}

void GDxUav::SetViewportAndScissorRect(UINT Width, UINT Height)
{
	mViewport.TopLeftX = 0.0f;
	mViewport.TopLeftY = 0.0f;
	mViewport.Width = (FLOAT)Width;
	mViewport.Height = (FLOAT)Height;
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, (int)(mViewport.Width), (int)(mViewport.Height) };
}

void GDxUav::SetDescriptorSize()
{
	SrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE GDxUav::GetGpuSrv()
{
	return mGpuSrv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE GDxUav::GetGpuUav()
{
	return mGpuUav;
}

D3D12_CPU_DESCRIPTOR_HANDLE GDxUav::GetCpuRtv()
{
	if (!bCreateRtv)
		ThrowGGiException("No rtv exists.");
	return mRtv->mRtvCpu;
}

UINT GDxUav::GetSize()
{
	return 2;
}

bool GDxUav::ResourceNotNull()
{
	return (mResource != nullptr);
}

ID3D12Resource* GDxUav::GetResource()
{
	return mResource.Get();
}

DXGI_FORMAT GDxUav::GetFormat()
{
	return mProperties.mUavFormat;
}

bool GDxUav::IsTexture()
{
	return bIsTexture;
}



