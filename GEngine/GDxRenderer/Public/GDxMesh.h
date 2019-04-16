#pragma once
#include "GDxPreInclude.h"
#include "GVertexIndexBuffer.h"


class GDxMesh : public GRiMesh
{
public:

	GDxMesh() = default;
	GDxMesh(const GDxMesh& rhs) = delete;
	~GDxMesh() = default;

	GDxMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<GRiMeshData> meshData);

	void Create(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<GRiMeshData> meshData);

	std::shared_ptr<GVertexIndexBuffer> mVIBuffer;

};

