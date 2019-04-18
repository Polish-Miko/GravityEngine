#include "stdafx.h"
#include "GDxStaticVIBuffer.h"
#include "GDxUtil.h"


GDxStaticVIBuffer::GDxStaticVIBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<GRiVertex> vertices, std::vector<uint32_t> indices) : GDxVertexIndexBuffer(device, cmdList, vertices, indices)
{
	Create(device, cmdList, vertices, indices);
}


void GDxStaticVIBuffer::Create(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<GRiVertex> vertices, std::vector<uint32_t> indices)
{
	ThrowIfFailed(D3DCreateBlob(VertexBufferByteSize, &VertexBufferCPU));
	CopyMemory(VertexBufferCPU->GetBufferPointer(), vertices.data(), VertexBufferByteSize);

	ThrowIfFailed(D3DCreateBlob(IndexBufferByteSize, &IndexBufferCPU));
	CopyMemory(IndexBufferCPU->GetBufferPointer(), indices.data(), IndexBufferByteSize);

	VertexBufferGPU = GDxUtil::CreateDefaultBuffer(device,
		cmdList, vertices.data(), VertexBufferByteSize, VertexBufferUploader);

	IndexBufferGPU = GDxUtil::CreateDefaultBuffer(device,
		cmdList, indices.data(), IndexBufferByteSize, IndexBufferUploader);
}

D3D12_INDEX_BUFFER_VIEW GDxStaticVIBuffer::IndexBufferView() const
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = IndexFormat;
	ibv.SizeInBytes = IndexBufferByteSize;

	return ibv;
}

D3D12_VERTEX_BUFFER_VIEW GDxStaticVIBuffer::VertexBufferView() const
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = VertexByteStride;
	vbv.SizeInBytes = VertexBufferByteSize;

	return vbv;
}

void GDxStaticVIBuffer::DisposeUploaders()
{
	VertexBufferUploader = nullptr;
	IndexBufferUploader = nullptr;
}