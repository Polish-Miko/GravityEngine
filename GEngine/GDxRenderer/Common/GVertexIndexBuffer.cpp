#include "stdafx.h"
#include "GVertexIndexBuffer.h"


GVertexIndexBuffer::GVertexIndexBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<GRiVertex> vertices, std::vector<uint32_t> indices)
{
	VertexByteStride = sizeof(GVertex);
	IndexFormat = DXGI_FORMAT_R32_UINT;
	VertexBufferByteSize = (UINT)vertices.size() * sizeof(GVertex);
	IndexBufferByteSize = (UINT)indices.size() * sizeof(std::uint32_t);
	VertexCount = (UINT)vertices.size();
	IndexCount = (UINT)indices.size();
}


