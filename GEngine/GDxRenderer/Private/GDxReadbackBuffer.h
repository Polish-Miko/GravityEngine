#pragma once
#include "GDxPreInclude.h"
#include "GDxUtil.h"



template<typename T>
class GDxReadbackBuffer
{

public:

	GDxReadbackBuffer() {}

	~GDxReadbackBuffer() {}

	GDxReadbackBuffer(const GDxReadbackBuffer& rhs) = delete;
	GDxReadbackBuffer& operator=(const GDxReadbackBuffer& rhs) = delete;

	GDxReadbackBuffer(ID3D12Device* device, UINT elementCount)
	{
		mElementByteSize = sizeof(T);
		mElementCount = elementCount;
		pDevice = device;

		D3D12_HEAP_PROPERTIES HeapProps;
		HeapProps.Type = D3D12_HEAP_TYPE_READBACK;
		HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		HeapProps.CreationNodeMask = 1;
		HeapProps.VisibleNodeMask = 1;

		ThrowIfFailed(device->CreateCommittedResource(
			&HeapProps,
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * mElementCount, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&mReadbackBuffer)));

		// We do not need to unmap until we are done with the resource.  However, we must not write to
		// the resource while it is in use by the GPU (so we must use synchronization techniques).
	}

	T* Map()
	{
		ThrowIfFailed(mReadbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
		return mMappedData;
	}

	void Unmap()
	{
		if (mReadbackBuffer != nullptr)
			mReadbackBuffer->Unmap(0, nullptr);
	}

	/*
	ID3D12Resource* Resource()const
	{
		return mReadbackBuffer.Get();
	}

	void CopyData(int elementIndex, const T& data)
	{
		memcpy(&mMappedData[elementIndex*mElementByteSize], &data, sizeof(T));
	}
	*/

	Microsoft::WRL::ComPtr<ID3D12Resource> mReadbackBuffer;

private:
	ID3D12Device* pDevice;
	T* mMappedData = nullptr;

	UINT mElementByteSize = 0;
	UINT mElementCount = 0;
};

