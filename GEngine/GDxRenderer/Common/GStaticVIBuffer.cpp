#include "stdafx.h"
#include "GStaticVIBuffer.h"
#include "GDX12Util.h"


GStaticVIBuffer::GStaticVIBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<GVertex> vertices, std::vector<uint32_t> indices) : GVertexIndexBuffer(device, cmdList, vertices, indices)
{
	Create(device, cmdList, vertices, indices);
}


void GStaticVIBuffer::Create(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<GVertex> vertices, std::vector<uint32_t> indices)
{
	ThrowIfFailed(D3DCreateBlob(VertexBufferByteSize, &VertexBufferCPU));
	CopyMemory(VertexBufferCPU->GetBufferPointer(), vertices.data(), VertexBufferByteSize);

	ThrowIfFailed(D3DCreateBlob(IndexBufferByteSize, &IndexBufferCPU));
	CopyMemory(IndexBufferCPU->GetBufferPointer(), indices.data(), IndexBufferByteSize);

	VertexBufferGPU = GDX12Util::CreateDefaultBuffer(device,
		cmdList, vertices.data(), VertexBufferByteSize, VertexBufferUploader);

	IndexBufferGPU = GDX12Util::CreateDefaultBuffer(device,
		cmdList, indices.data(), IndexBufferByteSize, IndexBufferUploader);
}

D3D12_INDEX_BUFFER_VIEW GStaticVIBuffer::IndexBufferView() const
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = IndexFormat;
	ibv.SizeInBytes = IndexBufferByteSize;

	return ibv;
}

D3D12_VERTEX_BUFFER_VIEW GStaticVIBuffer::VertexBufferView() const
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = VertexByteStride;
	vbv.SizeInBytes = VertexBufferByteSize;

	return vbv;
}

void GStaticVIBuffer::DisposeUploaders()
{
	VertexBufferUploader = nullptr;
	IndexBufferUploader = nullptr;
}