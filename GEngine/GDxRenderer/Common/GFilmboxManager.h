#pragma once

#include "GUtilInclude.h"
#include "GMesh.h"

class FbxDeleter {
public:
	void operator()(FbxManager *m) { m->Destroy(); }
	void operator()(FbxScene *s) { s->Destroy(); }
	void operator()(FbxImporter *i) { i->Destroy(); }
	void operator()(FbxIOSettings *io) { io->Destroy(); }
};

class GFilmboxManager
{
public:

	~GFilmboxManager(){}

	GFilmboxManager(const GFilmboxManager& rhs) = delete;

	GFilmboxManager& operator=(const GFilmboxManager& rhs) = delete;

	static GFilmboxManager& GetManager()
	{
		static GFilmboxManager *instance = new GFilmboxManager();
		return *instance;
	}

	bool ImportFbxFile_Mesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const char* FileName, GMesh* Mesh);

private:

	GFilmboxManager();

	std::unique_ptr<FbxManager, FbxDeleter> mManager;

	std::unique_ptr<FbxIOSettings, FbxDeleter> mIoSettings;

	bool ImportNode_Mesh(FbxNode* pNode, std::vector<MeshData>& meshData);

	bool ImportMesh(FbxNode* pNode, std::vector<MeshData>& meshData);

	template <typename TGeometryElement, typename TValue> TValue GetVertexElement(TGeometryElement* pElement, int iPoint, int iTriangle, int iVertex, TValue defaultValue);

	DirectX::XMFLOAT4X4 ToDxMatrix(const FbxAMatrix &fbxMat);

	FbxAMatrix GetGeometryTransform(FbxNode* pNode);

};

