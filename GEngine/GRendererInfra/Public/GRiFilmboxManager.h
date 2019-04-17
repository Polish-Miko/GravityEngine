#pragma once
#include "GRiPreInclude.h"
#include "GRiMeshData.h"
#include "GRiRendererFactory.h"


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

	GRiFilmboxManager();

	~GRiFilmboxManager() {}

	//GRiFilmboxManager(const GRiFilmboxManager& rhs) = delete;

	//GRiFilmboxManager(GRiRendererFactory* rFac);

	void SetRendererFactory(GRiRendererFactory* rFac);

	bool ImportFbxFile_Mesh(std::wstring FileName, std::vector<GRiMeshData>& outMeshDataList);

private:

	//GRiFilmboxManager();

	GRiRendererFactory* pRendererFactory;

	std::unique_ptr<FbxManager, FbxDeleter> mManager;

	std::unique_ptr<FbxIOSettings, FbxDeleter> mIoSettings;

	bool ImportNode_Mesh(FbxNode* pNode, std::vector<GRiMeshData>& outMeshDataList);

	bool ImportMesh(FbxNode* pNode, std::vector<GRiMeshData>& outMeshDataList);

	template <typename TGeometryElement, typename TValue> TValue GetVertexElement(TGeometryElement* pElement, int iPoint, int iTriangle, int iVertex, TValue defaultValue);

	GGiFloat4x4 ToGMatrix(const FbxAMatrix &fbxMat);

	FbxAMatrix GetGeometryTransform(FbxNode* pNode);
};

