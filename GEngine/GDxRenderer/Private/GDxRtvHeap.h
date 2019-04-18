#pragma once
//#include "GUtilInclude.h"
#include "GDxRtv.h"
#include "GDxDescriptorHeap.h"


class GDxRtvHeap
{
public:

	GDxRtvHeap() = delete;
	GDxRtvHeap(ID3D12Device* device, UINT ClientWidth, UINT ClientHeight,
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrv,
		std::vector<GRtvProperties> gRtvProperties);
	GDxRtvHeap(const GDxRtvHeap& rhs) = delete;
	GDxRtvHeap& operator=(const GDxRtvHeap& rhs) = delete;
	~GDxRtvHeap() = default;

	GDxDescriptorHeap mRtvHeap;
	//GDescriptorHeap mSrvHeap;

	std::vector<std::shared_ptr<GDxRtv>> mRtv;

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

