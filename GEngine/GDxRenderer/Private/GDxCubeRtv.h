#pragma once

#include "GDxRtv.h"
#include "GDxDescriptorHeap.h"

class GDxCubeRtv
{
public:
	GDxCubeRtv() = delete;

	GDxCubeRtv(ID3D12Device* device, UINT CubemapSize,
		//UINT MipLevels,
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrv,
		GRtvProperties gRtvProperties);

	GDxCubeRtv(const GDxCubeRtv& rhs) = delete;

	GDxCubeRtv& operator=(const GDxCubeRtv& rhs) = delete;

	~GDxCubeRtv() = default;

	void BuildResources();

	void BuildDescriptors();

	GDxDescriptorHeap mRtvHeap;

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

