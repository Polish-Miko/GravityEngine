#pragma once

#include "GRtv.h"
#include "GDescriptorHeap.h"

class GCubeRtv
{
public:
	GCubeRtv() = delete;

	GCubeRtv(ID3D12Device* device, UINT CubemapSize,
		//UINT MipLevels,
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrv,
		GRtvProperties gRtvProperties);

	GCubeRtv(const GCubeRtv& rhs) = delete;

	GCubeRtv& operator=(const GCubeRtv& rhs) = delete;

	~GCubeRtv() = default;

	void BuildResources();

	void BuildDescriptors();

	GDescriptorHeap mRtvHeap;

	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvGpu();

	DXGI_FORMAT mFormat;
	FLOAT mClearColor[4] = { 0,0,0,0 };

	Microsoft::WRL::ComPtr<ID3D12Resource> mResource;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	void SetViewportAndScissorRect(UINT CubemapSize);

private:

	ID3D12Device* md3dDevice;

	UINT mCubemapSize;

	//UINT mMipLevels;

	CD3DX12_CPU_DESCRIPTOR_HANDLE SrvCpu;
	CD3DX12_GPU_DESCRIPTOR_HANDLE SrvGpu;

	UINT SrvDescriptorSize;
	UINT RtvDescriptorSize;

	void SetDescriptorSize();
};

