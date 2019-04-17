#pragma once

#include "GVertexIndexBuffer.h"

class GStaticVIBuffer : public GVertexIndexBuffer
{
public:
	GStaticVIBuffer() = delete;
	~GStaticVIBuffer() = default;

	GStaticVIBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<GRiVertex> vertices, std::vector<uint32_t> indices);

	virtual void Create(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<GRiVertex> vertices, std::vector<uint32_t> indices) override;

	// System memory copies.  Use Blobs because the vertex/index format can be generic.
	// It is up to the client to cast appropriately.  
	Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	virtual D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const override;
	virtual D3D12_INDEX_BUFFER_VIEW IndexBufferView() const override;

	virtual void DisposeUploaders() override;
};

