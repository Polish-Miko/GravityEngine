#include "stdafx.h"
#include "GRiFilmboxManager.h"


GRiFilmboxManager::GRiFilmboxManager()
{
	//pRendererFactory = rFac;

	// Initialize the SDK manager. This object handles memory management.
	mManager = std::unique_ptr<FbxManager, FbxDeleter>(FbxManager::Create());

	// Create the IO settings object.
	mIoSettings = std::unique_ptr<FbxIOSettings, FbxDeleter>(FbxIOSettings::Create(mManager.get(), IOSROOT));

	mManager->SetIOSettings(mIoSettings.get());
}

bool GRiFilmboxManager::ImportFbxFile_Mesh(std::wstring FileName, std::vector<GRiMeshData>& outMeshDataList)
{
	const wchar_t *fileName_WC = FileName.c_str();

	// convert to UTF8
	char *fileName_UTF8;
	FbxWCToUTF8(fileName_WC, fileName_UTF8);
	//FbxString lSourceFileName(fileName_UTF8);
	//FbxAutoFreePtr<char> lsp(fileName_UTF8);

	// Create a new scene so that it can be populated by the imported file.
	std::unique_ptr<FbxScene, FbxDeleter> mScene(FbxScene::Create(mManager.get(), "TempScene"));

	// Create an importer using the SDK manager.
	//std::unique_ptr<FbxImporter> lImporter = std::make_unique<FbxImporter>(FbxImporter::Create(mManager.get(), ""));
	std::unique_ptr<FbxImporter, FbxDeleter> lImporter = std::unique_ptr<FbxImporter, FbxDeleter>(FbxImporter::Create(mManager.get(), ""));

	// Use the first argument as the filename for the importer.
	if (!lImporter->Initialize(fileName_UTF8, -1, mManager->GetIOSettings()))
	{
		std::string e = lImporter->GetStatus().GetErrorString();
		std::wstring we(e.length(), L' ');
		std::copy(e.begin(), e.end(), we.begin());
		ThrowGGiException("Fail to load fbx file.");
		return false;
	}

	// Import the contents of the file into the scene.
	lImporter->Import(mScene.get());

	// convert to centimeters
	if (mScene->GetGlobalSettings().GetSystemUnit() != FbxSystemUnit::cm)
		FbxSystemUnit::cm.ConvertScene(mScene.get());

	// triangulate
	FbxGeometryConverter GeometryConverter(mManager.get());
	GeometryConverter.Triangulate(mScene.get(), true, false);

	FbxNode* root = mScene->GetRootNode();
	if (!root)
	{
		ThrowGGiException("Scene has no root node when importing.");
		return false;
	}

	//if (!ImportNode_Material(root, outMeshDataList))
		//return false;

	if (!ImportNode_Mesh(root, outMeshDataList))
		return false;

	return true;
}

bool GRiFilmboxManager::ImportNode_Material(FbxNode* pNode, std::vector<GRiMeshData>& outMeshDataList)
{
	if (auto pMesh = pNode->GetMesh())
	{
		if (!ImportMaterial(pNode, outMeshDataList))
			return false;
	}

	for (int i = 0, e = pNode->GetChildCount(); i < e; i++)
	{
		if (!ImportNode_Material(pNode->GetChild(i), outMeshDataList))
		{
			return false;
		}
	}
	return true;
}

bool GRiFilmboxManager::ImportMaterial(FbxNode* pNode, std::vector<GRiMeshData>& outMeshDataList)
{
	auto materialCount = pNode->GetMaterialCount();

	for (int materialIndex = 0; materialIndex < materialCount; materialIndex++)
	{
		FbxSurfaceMaterial* pSurfaceMaterial = pNode->GetMaterial(materialIndex);

		auto cMatName = pSurfaceMaterial->GetName();
		std::string sMatName(cMatName);
		auto wMatName = GGiEngineUtil::StringToWString(sMatName);

		bool bFound = false;
		for (auto meshData : outMeshDataList)
		{
			if (meshData.SubmeshName == wMatName)
			{
				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			GRiMeshData newMeshData;
			newMeshData.SubmeshName = wMatName;
			newMeshData.Vertices.clear();
			newMeshData.Indices.clear();
			outMeshDataList.push_back(newMeshData);
		}
	}
}

bool GRiFilmboxManager::ImportNode_Mesh(FbxNode* pNode, std::vector<GRiMeshData>& outMeshDataList)
{
	if (auto pMesh = pNode->GetMesh())
	{
		if (!ImportMesh(pNode, outMeshDataList))
			return false;
	}

	for (int i = 0, e = pNode->GetChildCount(); i < e; i++)
	{
		if (!ImportNode_Mesh(pNode->GetChild(i), outMeshDataList))
		{
			return false;
		}
	}
	return true;
}

bool GRiFilmboxManager::ImportMesh(FbxNode* pNode, std::vector<GRiMeshData>& outMeshDataList)
{
	auto pMesh = pNode->GetMesh();	

	if (!pMesh->IsTriangleMesh())
	{
		printf("error: We only support triangle meshes.\n");
		return false;
	}

	uint32_t numTriangles = uint32_t(pMesh->GetPolygonCount());
	uint32_t numPoints = pMesh->GetControlPointsCount();

	FbxLayerElementArrayTemplate<int>* pMaterialIndices;
	FbxGeometryElement::EMappingMode   materialMappingMode = FbxGeometryElement::eNone;
	pMaterialIndices = &pMesh->GetElementMaterial()->GetIndexArray();
	materialMappingMode = pMesh->GetElementMaterial()->GetMappingMode();
	/*
	int* pTriangleMtlIndex = new int[numTriangles];
	if (pMesh->GetElementMaterial())
	{
		pMaterialIndices = &pMesh->GetElementMaterial()->GetIndexArray();
		materialMappingMode = pMesh->GetElementMaterial()->GetMappingMode();
		if (pMaterialIndices)
		{
			switch (materialMappingMode)
			{
			case FbxGeometryElement::eByPolygon:
			{
				if (pMaterialIndices->GetCount() == numTriangles)
				{
					for (int triangleIndex = 0; triangleIndex < numTriangles; ++triangleIndex)
					{
						int materialIndex = pMaterialIndices->GetAt(triangleIndex);

						pTriangleMtlIndex[triangleIndex] = materialIndex;
					}
				}
			}
			break;

			case FbxGeometryElement::eAllSame:
			{
				int lMaterialIndex = pMaterialIndices->GetAt(0);

				for (int triangleIndex = 0; triangleIndex < numTriangles; ++triangleIndex)
				{
					int materialIndex = pMaterialIndices->GetAt(triangleIndex);

					pTriangleMtlIndex[triangleIndex] = materialIndex;
				}
			}
			}
		}
	}
	*/

	//GRiMeshData mdata;

	//mdata.SubmeshName = GGiEngineUtil::StringToWString(std::string(pNode->GetName()));
	//mdata.Vertices = std::vector<GRiVertex>(numTriangles * 3);
	//mdata.Indices = std::vector<uint32_t>(numTriangles * 3);

	auto materialCount = pNode->GetMaterialCount();
	auto mdata = new GRiMeshData*[materialCount];

	for (int materialIndex = 0; materialIndex < materialCount; materialIndex++)
	{
		FbxSurfaceMaterial* pSurfaceMaterial = pNode->GetMaterial(materialIndex);

		auto cMatName = pSurfaceMaterial->GetName();
		std::string sMatName(cMatName);
		auto wMatName = GGiEngineUtil::StringToWString(sMatName);

		bool bFound = false;
		for (auto meshData : outMeshDataList)
		{
			if (meshData.SubmeshName == wMatName)
			{
				mdata[materialIndex] = &meshData;
				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			GRiMeshData newMeshData;
			newMeshData.SubmeshName = wMatName;
			newMeshData.Vertices.clear();
			newMeshData.Indices.clear();
			outMeshDataList.push_back(newMeshData);
			mdata[materialIndex] = &outMeshDataList[outMeshDataList.size() - 1];
		}
	}

	FbxAMatrix submeshTrans = pNode->EvaluateGlobalTransform() * GetGeometryTransform(pNode);

	GGiFloat4x4* vertTrans = ToGMatrix(submeshTrans);

	pMesh->GenerateNormals();
	const FbxGeometryElementNormal* pNormals = pMesh->GetElementNormal(0);

	pMesh->GenerateTangentsDataForAllUVSets();
	const FbxGeometryElementTangent* pTangents = pMesh->GetElementTangent(0);

	const FbxGeometryElementUV* pUVs = pMesh->GetElementUV(0);

	if (!pMesh->GetElementMaterial() || !pMaterialIndices)
		ThrowGGiException("Material not found!");

	if (materialMappingMode == FbxGeometryElement::eByPolygon && pMaterialIndices->GetCount() != numTriangles)
		ThrowGGiException("Material indices matching error!");

	for (uint32_t t = 0; t < numTriangles; t++)
	{
		int materialIndex = 0;
		switch (materialMappingMode)
		{
		case FbxGeometryElement::eByPolygon:
			materialIndex = pMaterialIndices->GetAt(t);
			break;

		case FbxGeometryElement::eAllSame:
			materialIndex = pMaterialIndices->GetAt(t);
			break;

		default:
			ThrowGGiException("Material mapping mode error!");
		}

		for (uint32_t v = 0; v < 3; v++)
		{
			int iPoint = pMesh->GetPolygonVertex(t, v);
			int index = mdata[materialIndex]->Indices.size();
			mdata[materialIndex]->Indices.push_back(index);

			FbxVector4 position = pMesh->GetControlPointAt(iPoint);
			FbxVector4 normal = GetVertexElement(pNormals, iPoint, t, v, FbxVector4(0, 0, 0, 0));
			FbxVector4 tangent = GetVertexElement(pTangents, iPoint, t, v, FbxVector4(0, 0, 0, 0));
			FbxVector2 uv = GetVertexElement(pUVs, iPoint, t, v, FbxVector2(0, 0));

			// Transform the vertex.
			GGiFloat4* vertPos = pRendererFactory->CreateFloat4(float(position[0]), float(position[1]), float(position[2]), 1.0f);
			GGiFloat4* finalVertPos = &((*vertPos) * (*vertTrans));

			GRiVertex vertex;
			vertex.Position[0] = finalVertPos->GetX();
			vertex.Position[1] = finalVertPos->GetY();
			vertex.Position[2] = finalVertPos->GetZ();
			vertex.Normal[0] = (float)normal[0];
			vertex.Normal[1] = (float)normal[1];
			vertex.Normal[2] = (float)normal[2];
			vertex.TangentU[0] = (float)tangent[0];
			vertex.TangentU[1] = (float)tangent[1];
			vertex.TangentU[2] = (float)tangent[2];
			vertex.UV[0] = (float)uv[0];
			vertex.UV[1] = 1.0f - (float)uv[1];

			mdata[materialIndex]->Vertices[index] = vertex;

			/*
			int iPoint = pMesh->GetPolygonVertex(t, v);
			mdata.Indices[3 * t + v] = 3 * t + v;

			FbxVector4 position = pMesh->GetControlPointAt(iPoint);
			FbxVector4 normal = GetVertexElement(pNormals, iPoint, t, v, FbxVector4(0, 0, 0, 0));
			FbxVector4 tangent = GetVertexElement(pTangents, iPoint, t, v, FbxVector4(0, 0, 0, 0));
			FbxVector2 uv = GetVertexElement(pUVs, iPoint, t, v, FbxVector2(0, 0));

			// Transform the vertex.
			GGiFloat4* vertPos = pRendererFactory->CreateFloat4(float(position[0]), float(position[1]), float(position[2]), 1.0f);
			GGiFloat4* finalVertPos = &((*vertPos) * (*vertTrans));

			GRiVertex vertex;
			vertex.Position[0] = finalVertPos->GetX();
			vertex.Position[1] = finalVertPos->GetY();
			vertex.Position[2] = finalVertPos->GetZ();
			vertex.Normal[0] = (float)normal[0];
			vertex.Normal[1] = (float)normal[1];
			vertex.Normal[2] = (float)normal[2];
			vertex.TangentU[0] = (float)tangent[0];
			vertex.TangentU[1] = (float)tangent[1];
			vertex.TangentU[2] = (float)tangent[2];
			vertex.UV[0] = (float)uv[0];
			vertex.UV[1] = 1.0f - (float)uv[1];

			mdata.Vertices[3 * t + v] = vertex;
			*/
		}

	}

	//outMeshDataList.push_back(mdata);

	return true;
}

template <typename TGeometryElement, typename TValue>
TValue GRiFilmboxManager::GetVertexElement(TGeometryElement* pElement, int iPoint, int iTriangle, int iVertex, TValue defaultValue) {
	if (!pElement || pElement->GetMappingMode() == FbxGeometryElement::eNone)
		ThrowGGiException("FbxGeometryElement mapping mode error.")
		int index = 0;

	if (pElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
		index = iPoint;
	//else if (pElement->GetMappingMode() == FbxGeometryElement::eByPolygon)
		//index = iTriangle;
	else if (pElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
		index = iTriangle * 3 + iVertex;
	else
		ThrowGGiException("FbxGeometryElement mapping mode error.")

		if (pElement->GetReferenceMode() != FbxGeometryElement::eDirect)
			index = pElement->GetIndexArray().GetAt(index);

	return pElement->GetDirectArray().GetAt(index);
}

GGiFloat4x4* GRiFilmboxManager::ToGMatrix(const FbxAMatrix& fbxMat)
{
	FbxVector4 t = fbxMat.GetT();
	FbxVector4 r = fbxMat.GetR();
	FbxVector4 s = fbxMat.GetS();

	t.Set(-t.mData[0], t.mData[1], t.mData[2]);
	r.Set(r.mData[0], -r.mData[1], -r.mData[2]);

	GGiFloat4x4* mT = pRendererFactory->CreateFloat4x4();
	GGiFloat4x4* mR = pRendererFactory->CreateFloat4x4();
	GGiFloat4x4* mS = pRendererFactory->CreateFloat4x4();

	mT->SetByTranslation(float(t[0]), float(t[1]), float(t[2]));
	mR->SetByRotationPitchYawRoll(float(r[0]) * GGiEngineUtil::PI / 180, float(r[1]) * GGiEngineUtil::PI / 180, float(r[2]) * GGiEngineUtil::PI / 180);
	mS->SetByScale(float(s[0]), float(s[1]), float(s[2]));


	GGiFloat4x4* mTrans = pRendererFactory->CreateFloat4x4();

	*mTrans = (*mT) * (*mR) * (*mS);

	return mTrans;
}

FbxAMatrix GRiFilmboxManager::GetGeometryTransform(FbxNode* pNode)
{
	if (!pNode)
	{
		ThrowGGiException("Null for mesh geometry");
	}

	const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

	return FbxAMatrix(lT, lR, lS);
}

void GRiFilmboxManager::SetRendererFactory(GRiRendererFactory* rFac)
{
	pRendererFactory = rFac;
}