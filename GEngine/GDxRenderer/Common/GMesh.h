#pragma once

#include "GUtilInclude.h"
#include "UploadBuffer.h"
#include "GVertexIndexBuffer.h"
#include "GVertex.h"

using namespace DirectX;

using uint16 = std::uint16_t;
using uint32 = std::uint32_t;

struct MeshData
{
	std::string SubmeshName;

	//DirectX::XMFLOAT4X4 Transform;

	std::vector<GVertex> Vertices;
	std::vector<uint32> Indices32;

	std::vector<uint16>& GetIndices16()
	{
		if (mIndices16.empty())
		{
			mIndices16.resize(Indices32.size());
			for (size_t i = 0; i < Indices32.size(); ++i)
				mIndices16[i] = static_cast<uint16>(Indices32[i]);
		}

		return mIndices16;
	}

private:
	std::vector<uint16> mIndices16;
};

struct GSubmesh
{
	UINT IndexCount = 0;

	//DirectX::XMFLOAT4X4 Transform;

	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

	// Bounding box of the geometry defined by this submesh. 
	// This is used in later chapters of the book.
	DirectX::BoundingBox Bounds;
};

class GMesh
{
public:

	GMesh() = default;
	GMesh(const GMesh& rhs) = delete;
	~GMesh() = default;

	GMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<MeshData> meshData);

	void Create(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<MeshData> meshData);

	// Give it a name so we can look it up by name.
	std::string Name;

	std::shared_ptr<GVertexIndexBuffer> mVIBuffer;

	bool bIsSkeletalMesh = false;

	// A MeshGeometry may store multiple geometries in one vertex/index buffer.
	// Use this container to define the Submesh geometries so we can draw
	// the Submeshes individually.
	std::unordered_map<std::string, GSubmesh> Submeshes;

	// We can free this memory after we finish upload to the GPU.
	//void DisposeUploaders();
};

