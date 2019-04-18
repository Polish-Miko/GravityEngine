#include "stdafx.h"
#include "GDxMesh.h"
#include "GDxStaticVIBuffer.h"





GDxMesh::GDxMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<GRiMeshData> meshData)
{
	Create(device, cmdList, meshData);
}

void GDxMesh::Create(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<GRiMeshData> meshData)
{
	UINT vertexOffset = 0;
	UINT indexOffset = 0;
	std::vector<GRiVertex> vertices;
	std::vector<std::uint32_t> indices;
	for (auto mdata : meshData)
	{
		GRiSubmesh submesh;
		submesh.IndexCount = (UINT)mdata.Indices.size();
		submesh.StartIndexLocation = indexOffset;
		submesh.BaseVertexLocation = vertexOffset;
		//submesh.Transform = mdata.Transform;

		vertices.insert(vertices.end(), std::begin(mdata.Vertices), std::end(mdata.Vertices));
		indices.insert(indices.end(), std::begin(mdata.Indices), std::end(mdata.Indices));

		Submeshes[mdata.SubmeshName] = submesh;

		vertexOffset += (UINT)mdata.Vertices.size();
		indexOffset += (UINT)mdata.Indices.size();
	}

	mVIBuffer = std::make_shared<GDxStaticVIBuffer>(device, cmdList, vertices, indices);
	if (mVIBuffer == nullptr)
		ThrowDxException(L"GStaticVIBuffer Cast Fail");

}

