#pragma once

//#include "GUtilInclude.h"
#include "GDxPreInclude.h"
#include "GVertex.h"

class GVertexIndexBuffer
{
public:
	GVertexIndexBuffer() = delete;
	virtual ~GVertexIndexBuffer() {}

	GVertexIndexBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<GVertex> vertices, std::vector<uint32_t> indices);

	virtual void Create(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<GVertex> vertices, std::vector<uint32_t> indices) = 0;

	// Data about the buffers.
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	UINT VertexCount = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R32_UINT;
	UINT IndexBufferByteSize = 0;
	UINT IndexCount = 0;

	virtual D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const = 0;

	virtual D3D12_INDEX_BUFFER_VIEW IndexBufferView() const = 0;

	virtual void DisposeUploaders() = 0;
};

