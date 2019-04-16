#pragma once
#include "GRiPreInclude.h"
#include "GRiMeshData.h"


class FbxDeleter {
public:
	void operator()(FbxManager *m) { m->Destroy(); }
	void operator()(FbxScene *s) { s->Destroy(); }
	void operator()(FbxImporter *i) { i->Destroy(); }
	void operator()(FbxIOSettings *io) { io->Destroy(); }
};


class GRiFilmboxManager
{

public: 

	//GRiFilmboxManager();

	~GRiFilmboxManager(){}

	GRiFilmboxManager(const GRiFilmboxManager& rhs) = delete;

	static GRiFilmboxManager& GetManager()
	{
		static GRiFilmboxManager *instance = new GRiFilmboxManager();
		return *instance;
	}

	bool ImportFbxFile_Mesh(std::wstring* FileName, std::vector<GRiMeshData> MeshDataList);

private:

	//GRiFilmboxManager();

	std::unique_ptr<FbxManager, FbxDeleter> mManager;

	std::unique_ptr<FbxIOSettings, FbxDeleter> mIoSettings;

	bool ImportNode_Mesh(FbxNode* pNode, std::vector<GRiMeshData>& meshData);

	bool ImportMesh(FbxNode* pNode, std::vector<GRiMeshData>& meshData);

	template <typename TGeometryElement, typename TValue> TValue GetVertexElement(TGeometryElement* pElement, int iPoint, int iTriangle, int iVertex, TValue defaultValue);

	//DirectX::XMFLOAT4X4 ToDxMatrix(const FbxAMatrix &fbxMat);

	FbxAMatrix GetGeometryTransform(FbxNode* pNode);
};

