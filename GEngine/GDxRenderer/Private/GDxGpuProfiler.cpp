#include "stdafx.h"
#include "GDxGpuProfiler.h"


GDxGpuProfiler::GDxGpuProfiler()
{
	;
}

GDxGpuProfiler::~GDxGpuProfiler()
{
	;
}

GDxGpuProfiler& GDxGpuProfiler::GetGpuProfiler()
{
	static GDxGpuProfiler *instance = new GDxGpuProfiler();
	return *instance;
}

void GDxGpuProfiler::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12CommandQueue* cmdQueue)
{
	pDevice = device;
	pCommandList = cmdList;
	pCommandQueue = cmdQueue;

	D3D12_QUERY_HEAP_DESC heapDesc = { };
	heapDesc.Count = MAX_GPU_PROFILE_NUM * 2;
	heapDesc.NodeMask = 0;
	heapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	pDevice->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&queryHeap));

	readbackBuffer = std::make_unique<GDxReadbackBuffer<UINT64>>(pDevice, MAX_GPU_PROFILE_NUM * 2);
}

void GDxGpuProfiler::StartGpuProfile(std::string profileName)
{
	//if (bActive == true)
		//ThrowGGiException("Cannot start two gpu profile query at the same time.");

	if (mProfileNameList.size() >= MAX_GPU_PROFILE_NUM)
		ThrowGGiException("Cannot start more than MAX_GPU_PROFILE_NUM gpu profiles.");

	//bActive = true;

	mProfileNameList.push_back(profileName);

	// Insert the start timestamp
	pCommandList->EndQuery(queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, (UINT)(mProfileNameList.size() - 1) * 2);
}

void GDxGpuProfiler::EndGpuProfile(std::string profileName)
{
	//if (bActive == false)
		//ThrowGGiException("Cannot end gpu profile query if no one has been started.");

	// Found profile in the name list
	size_t i = 0;
	for (; i < mProfileNameList.size(); i++)
	{
		if (mProfileNameList[i] == profileName)
			break;
	}
	if (i >= mProfileNameList.size())
		ThrowGGiException("Gpu profile not found.")

	// Insert the end timestamp

	UINT startQueryIdx = UINT(i * 2);
	UINT endQueryIdx = startQueryIdx + 1;
	pCommandList->EndQuery(queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, endQueryIdx);

	// Resolve the data
	UINT64 dstOffset = startQueryIdx * sizeof(UINT64);
	pCommandList->ResolveQueryData(queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, startQueryIdx, 2, readbackBuffer->mReadbackBuffer.Get(), dstOffset);

	//bActive = false;
}

void GDxGpuProfiler::BeginFrame()
{
	mProfileNameList.clear();
	mProfiles.clear();
}

void GDxGpuProfiler::EndFrame()
{
	UINT64 freq;
	ThrowIfFailed(pCommandQueue->GetTimestampFrequency(&freq));

	const UINT64* queryData = readbackBuffer->Map();

	for (auto i = 0u; i < mProfileNameList.size(); i++)
	{
		UINT startQueryIdx = UINT(i * 2);
		UINT endQueryIdx = startQueryIdx + 1;

		UINT64 startTime = queryData[startQueryIdx];
		UINT64 endTime = queryData[endQueryIdx];

		UINT64 delta = endTime - startTime;
		double frequency = double(freq);
		float deltaTime = (float)((delta / frequency) * 1000.0);

		ProfileData profileData;
		profileData.name = mProfileNameList[i];
		profileData.time = deltaTime;
		mProfiles.push_back(profileData);
	}

	readbackBuffer->Unmap();
}

float GDxGpuProfiler::GetProfileByName(std::string name)
{
	return -1.f;
}

std::vector<ProfileData> GDxGpuProfiler::GetProfiles()
{
	return mProfiles;
}




