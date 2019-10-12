#pragma once
#include "GDxPreInclude.h"
#include "GDxReadbackBuffer.h"



#define MAX_GPU_PROFILE_NUM 128



class GDxGpuProfiler
{

public:

	GDxGpuProfiler(const GDxGpuProfiler& rhs) = delete;
	GDxGpuProfiler& operator=(const GDxGpuProfiler& rhs) = delete;
	~GDxGpuProfiler();

	static GDxGpuProfiler& GetGpuProfiler();

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12CommandQueue* cmdQueue);

	void StartGpuProfile(std::string profileName);
	void EndGpuProfile(std::string profileName);

	float GetProfileByName(std::string name);
	std::vector<ProfileData> GetProfiles();

	void BeginFrame();
	void EndFrame();

protected:

	std::vector<std::string> mProfileNameList;
	Microsoft::WRL::ComPtr<ID3D12QueryHeap> queryHeap = nullptr;
	std::unique_ptr<GDxReadbackBuffer<UINT64>> readbackBuffer;
	//bool bActive = false;

	ID3D12Device* pDevice = nullptr;
	ID3D12GraphicsCommandList* pCommandList = nullptr;
	ID3D12CommandQueue* pCommandQueue = nullptr;

	std::vector<ProfileData> mProfiles;

private:

	GDxGpuProfiler();

};

