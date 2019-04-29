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

	// Generate bounding box.
	float vMax[3] = { 0,0,0 };
	float vMin[3] = { 0,0,0 };
	for (auto v : vertices)
	{
		if (v.Position[0] > vMax[0])
			vMax[0] = v.Position[0];
		if (v.Position[1] > vMax[1])
			vMax[1] = v.Position[1];
		if (v.Position[2] > vMax[2])
			vMax[2] = v.Position[2];
		if (v.Position[0] < vMin[0])
			vMin[0] = v.Position[0];
		if (v.Position[1] < vMin[1])
			vMin[1] = v.Position[1];
		if (v.Position[2] < vMin[2])
			vMin[2] = v.Position[2];
	}
	bounds.Center[0] = (vMax[0] + vMin[0]) / 2;
	bounds.Center[1] = (vMax[1] + vMin[1]) / 2;
	bounds.Center[2] = (vMax[2] + vMin[2]) / 2;
	bounds.Extents[0] = (vMax[0] - vMin[0]) / 2;
	bounds.Extents[1] = (vMax[1] - vMin[1]) / 2;
	bounds.Extents[2] = (vMax[2] - vMin[2]) / 2;

	mVIBuffer = std::make_shared<GDxStaticVIBuffer>(device, cmdList, vertices, indices);
	if (mVIBuffer == nullptr)
		ThrowDxException(L"GStaticVIBuffer Cast Fail");

}

