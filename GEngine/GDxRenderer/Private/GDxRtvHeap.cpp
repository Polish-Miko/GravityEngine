#include "stdafx.h"
#include "GDxRtvHeap.h"


/*
GRtvHeap::GRtvHeap()
{
	SetDescriptorSize();
}
*/

GDxRtvHeap::GDxRtvHeap(ID3D12Device* device, UINT ClientWidth, UINT ClientHeight,
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrv, 
	std::vector<GRtvProperties> gRtvProperties)
{
	md3dDevice = device;
	mClientWidth = ClientWidth;
	mClientHeight = ClientHeight;
	SrvCpuStart = cpuSrv;
	SrvGpuStart = gpuSrv;
	SetDescriptorSize();
	for (size_t i = 0; i < gRtvProperties.size(); i++)
	{
		std::shared_ptr<GDxRtv> vRtv = std::make_shared<GDxRtv>(gRtvProperties[i], ClientWidth, ClientHeight);
		mRtv.push_back(vRtv);
	}
	ThrowIfFailed(mRtvHeap.Create(md3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, (UINT)(mRtv.size())));
	BuildResources();
	BuildDescriptors();
}

void GDxRtvHeap::BuildDescriptors()
{
	// Create RTVs.
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	ZeroMemory(&rtvDesc, sizeof(rtvDesc));
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (size_t i = 0; i < mRtv.size(); i++)
	{
		rtvDesc.Format = mRtv[i]->mProperties.mRtvFormat;
		md3dDevice->CreateRenderTargetView(mRtv[i]->mResource.Get(), &rtvDesc, mRtvHeap.handleCPU((UINT)i));
		mRtv[i]->mRtvCpu = mRtvHeap.handleCPU((UINT)i);
	}

	// Create SRVs.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;

	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	//ThrowIfFailed(mSrvHeap.Create(md3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mRtv.size()));
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpu(SrvCpuStart);

	for (size_t i = 0; i < mRtv.size(); i++)
	{
		srvDesc.Format = mRtv[i]->mProperties.mRtvFormat;
		srvDesc.Texture2D.MipLevels = mRtv[i]->mResource->GetDesc().MipLevels;
		//md3dDevice->CreateShaderResourceView(mRtv[i]->mResource.Get(), &srvDesc, mSrvHeap.handleCPU(i));
		md3dDevice->CreateShaderResourceView(mRtv[i]->mResource.Get(), &srvDesc, srvCpu);
		srvCpu.Offset(1, SrvDescriptorSize);
	}
}

void GDxRtvHeap::BuildResources()
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
	for (auto rtv : mRtv)
	{
		rtv->mResource = nullptr;

		texDesc.Width = (UINT)(max(rtv->mProperties.mWidthPercentage * mClientWidth, 1.0f));
		texDesc.Height = (UINT)(max(rtv->mProperties.mHeightPercentage * mClientHeight, 1.0f));
		texDesc.Format = rtv->mProperties.mRtvFormat;
		float normalClearColor[4];
		normalClearColor[0] = rtv->mProperties.mClearColor[0];
		normalClearColor[1] = rtv->mProperties.mClearColor[1];
		normalClearColor[2] = rtv->mProperties.mClearColor[2];
		normalClearColor[3] = rtv->mProperties.mClearColor[3];
		CD3DX12_CLEAR_VALUE optClear(rtv->mProperties.mRtvFormat, normalClearColor);
		
		ThrowIfFailed(md3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			&optClear,
			IID_PPV_ARGS(&rtv->mResource)));
	}
}

void GDxRtvHeap::OnResize(UINT newWidth, UINT newHeight)
{
	if (mClientWidth != newWidth || mClientHeight != newHeight)
	{
		mClientWidth = newWidth;
		mClientHeight = newHeight;

		for (auto rtv : mRtv)
		{
			rtv->OnResize(newWidth, newHeight);
		}

		BuildResources();
		BuildDescriptors();
	}
}

void GDxRtvHeap::SetDescriptorSize()
{
	SrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	RtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE GDxRtvHeap::GetSrvGpuStart()
{
	return SrvGpuStart;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE GDxRtvHeap::GetSrvGpu(int index)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpu(SrvGpuStart);
	srvGpu.Offset(index, SrvDescriptorSize);
	return srvGpu;
}



