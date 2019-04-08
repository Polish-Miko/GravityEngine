#pragma once
//#include "GUtilInclude.h"
#include "GRtv.h"
#include "GDescriptorHeap.h"


class GRtvHeap
{
public:

	GRtvHeap() = delete;
	GRtvHeap(ID3D12Device* device, UINT ClientWidth, UINT ClientHeight,
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrv,
		std::vector<GRtvProperties> gRtvProperties);
	GRtvHeap(const GRtvHeap& rhs) = delete;
	GRtvHeap& operator=(const GRtvHeap& rhs) = delete;
	~GRtvHeap() = default;

	GDescriptorHeap mRtvHeap;
	//GDescriptorHeap mSrvHeap;

	std::vector<std::shared_ptr<GRtv>> mRtv;

	void BuildResources();

	void BuildDescriptors();

	void OnResize(UINT newWidth, UINT newHeight);

	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvGpuStart();

private:

	ID3D12Device* md3dDevice;

	UINT mClientWidth;
	UINT mClientHeight;

	CD3DX12_CPU_DESCRIPTOR_HANDLE SrvCpuStart;
	CD3DX12_GPU_DESCRIPTOR_HANDLE SrvGpuStart;

	UINT SrvDescriptorSize;
	UINT RtvDescriptorSize;

	void SetDescriptorSize();

};

