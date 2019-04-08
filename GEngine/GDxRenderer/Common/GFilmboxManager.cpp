#include "stdafx.h"
#include "GFilmboxManager.h"
#include "GSceneObject.h"


GFilmboxManager::GFilmboxManager()
{
	// Initialize the SDK manager. This object handles memory management.
	mManager = std::unique_ptr<FbxManager, FbxDeleter>(FbxManager::Create());

	// Create the IO settings object.
	mIoSettings = std::unique_ptr<FbxIOSettings, FbxDeleter>(FbxIOSettings::Create(mManager.get(), IOSROOT));
	
	mManager->SetIOSettings(mIoSettings.get());
}

bool GFilmboxManager::ImportFbxFile_Mesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const char* FileName, GMesh* Mesh)
{
	// Create a new scene so that it can be populated by the imported file.
	std::unique_ptr<FbxScene, FbxDeleter> mScene(FbxScene::Create(mManager.get(), "TempScene"));

	// Create an importer using the SDK manager.
	//std::unique_ptr<FbxImporter> lImporter = std::make_unique<FbxImporter>(FbxImporter::Create(mManager.get(), ""));
	std::unique_ptr<FbxImporter, FbxDeleter> lImporter = std::unique_ptr<FbxImporter, FbxDeleter>(FbxImporter::Create(mManager.get(), ""));

	// Use the first argument as the filename for the importer.
	if (!lImporter->Initialize(FileName, -1, mManager->GetIOSettings()))
	{
		std::string e = lImporter->GetStatus().GetErrorString();
		std::wstring we(e.length(), L' ');
		std::copy(e.begin(), e.end(), we.begin());
		ThrowDxException(we);
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
		ThrowDxException(L"Scene has no root node when importing.");
		return false;
	}

	//ImportState state(model);
	//if (!importNode(state, root))
	std::vector<MeshData> meshData;

	if (!ImportNode_Mesh(root, meshData))
		return false;

	Mesh->Create(device, cmdList, meshData);

	return true;
}

bool GFilmboxManager::ImportNode_Mesh(FbxNode* pNode, std::vector<MeshData>& meshData)
{
	if (auto pMesh = pNode->GetMesh())
	{
		if (!ImportMesh(pNode, meshData))
			return false;
	}

	for (int i = 0, e = pNode->GetChildCount(); i < e; i++)
	{
		if (!ImportNode_Mesh(pNode->GetChild(i), meshData))
		{
			return false;
		}
	}
	return true;
}

bool GFilmboxManager::ImportMesh(FbxNode* pNode, std::vector<MeshData>& meshData)
{
	auto pMesh = pNode->GetMesh();
	if (!pMesh->IsTriangleMesh())
	{
		printf("error: We only support triangle meshes.\n");
		return false;
	}

	uint32_t numTriangles = uint32_t(pMesh->GetPolygonCount());
	uint32_t numPoints = pMesh->GetControlPointsCount();

	MeshData mdata;

	//std::vector<bool> bVertexLoaded(numPoints, false);

	mdata.SubmeshName = pNode->GetName();
	mdata.Vertices = std::vector<GVertex>(numTriangles * 3);
	mdata.Indices32 = std::vector<uint32>(numTriangles * 3);

	//model_import::Mesh mesh;
	FbxAMatrix submeshTrans = pNode->EvaluateGlobalTransform() * GetGeometryTransform(pNode);
	
	//mdata.Transform = ToDxMatrix(submeshTrans);
	DirectX::XMFLOAT4X4 vertTrans = ToDxMatrix(submeshTrans);

	//mdata.Transform = MathHelper::Identity4x4();
	// Import the materials.
	//int materialCount = pNode->GetMaterialCount();
	//for (int n = 0; n < materialCount; n++)
	//{
	//	FbxSurfaceMaterial* material = pNode->GetMaterial(n);
	//	if (!importMaterial(state, mesh, material)) {
	//		return false;
	//	}
	//}

	/*
	const FbxGeometryElementNormal* pNormals = pMesh->GetElementNormal(0);
	if (!pNormals)
	{
		// Generate normals if we don't have any
		pMesh->GenerateNormals();
		pNormals = pMesh->GetElementNormal(0);
	}
	*/

	pMesh->GenerateNormals();
	const FbxGeometryElementNormal* pNormals = pMesh->GetElementNormal(0);

	/*
	const FbxGeometryElementTangent* pTangents = pMesh->GetElementTangent(0);
	if (!pTangents)
	{
		pMesh->GenerateTangentsDataForAllUVSets();
		pTangents = pMesh->GetElementTangent(0);
	}
	*/

	pMesh->GenerateTangentsDataForAllUVSets();
	const FbxGeometryElementTangent* pTangents = pMesh->GetElementTangent(0);

	const FbxGeometryElementUV* pUVs = pMesh->GetElementUV(0);
	
	//const FbxLayerElementMaterial* pPolygonMaterials = pMesh->GetElementMaterial();
	//assert(pPolygonMaterials != nullptr);
	//assert(pPolygonMaterials->GetReferenceMode() == FbxGeometryElement::eIndex ||
	//	pPolygonMaterials->GetReferenceMode() == FbxGeometryElement::eIndexToDirect);
	//FbxGeometryElement::EMappingMode mappingMode = pPolygonMaterials->GetMappingMode();
	//auto getMaterialIndex = [pPolygonMaterials, mappingMode, materialCount](uint32_t triangleIndex) {
	//	int lookupIndex = 0;
	//	switch (mappingMode) {
	//	case FbxGeometryElement::eByPolygon:
	//		lookupIndex = triangleIndex;
	//		break;
	//	case FbxGeometryElement::eAllSame:
	//		lookupIndex = 0;
	//		break;
	//	default:
	//		assert(false);
	//		break;
	//	}

	//	int materialIndex = pPolygonMaterials->mIndexArray->GetAt(lookupIndex);
	//	assert(materialIndex >= 0 && materialIndex < materialCount);
	//	return uint32_t(materialIndex);
	//};
	
	// vertex deduplication
	//UnorderedMapGenerator<hvvr::ShadingVertex, uint32_t>::Type hashMap;

	//mesh.data.verts.reserve(numTriangles * 3);
	//mesh.data.triShade.resize(numTriangles);

	for (uint32_t t = 0; t < numTriangles; t++)
	{
		//uint32_t triIndices[3];
		for (uint32_t v = 0; v < 3; v++)
		{
			int iPoint = pMesh->GetPolygonVertex(t, v);
			mdata.Indices32[3 * t + v] = 3 * t + v;

			/*
			if (bVertexLoaded[iPoint])
			{
				continue;
			}
			*/

			FbxVector4 position = pMesh->GetControlPointAt(iPoint);
			FbxVector4 normal = GetVertexElement(pNormals, iPoint, t, v, FbxVector4(0, 0, 0, 0));
			FbxVector4 tangent = GetVertexElement(pTangents, iPoint, t, v, FbxVector4(0, 0, 0, 0));
			FbxVector2 uv = GetVertexElement(pUVs, iPoint, t, v, FbxVector2(0, 0));

			// Transform the vertex.
			DirectX::XMFLOAT4 vertPos(float(position[0]), float(position[1]), float(position[2]), 1.0f);
			DirectX::XMVECTOR finalVertPos = DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&vertPos), DirectX::XMLoadFloat4x4(&vertTrans));
			DirectX::XMStoreFloat4(&vertPos, finalVertPos);

			GVertex vertex;
			//vertex.Position = XMFLOAT3(float(position[0]), float(position[1]), float(position[2]));
			vertex.Position = XMFLOAT3(vertPos.x, vertPos.y, vertPos.z);
			vertex.Normal = XMFLOAT3(float(normal[0]), float(normal[1]), float(normal[2]));
			vertex.TangentU = XMFLOAT3(float(tangent[0]), float(tangent[1]), float(tangent[2]));
			vertex.UV = XMFLOAT2(float(uv[0]), 1.0f - float(uv[1]));

			mdata.Vertices[3 * t + v] = vertex;
			//bVertexLoaded[iPoint] = true;
		}

		
		//uint32_t materialIndex = getMaterialIndex(t);

		//hvvr::PrecomputedTriangleShade& triShade = mesh.data.triShade[t];
		//triShade.indices[0] = triIndices[0];
		//triShade.indices[1] = triIndices[1];
		//triShade.indices[2] = triIndices[2];
		//triShade.material = materialIndex;
		
	}

	meshData.push_back(mdata);


	//hvvr::GenerateTopology(mesh.data);

	//state.model.meshes.emplace_back(std::move(mesh));

	return true;
}

template <typename TGeometryElement, typename TValue>
TValue GFilmboxManager::GetVertexElement(TGeometryElement* pElement, int iPoint, int iTriangle, int iVertex, TValue defaultValue) {
	if (!pElement || pElement->GetMappingMode() == FbxGeometryElement::eNone)
		ThrowDxException(L"FbxGeometryElement mapping mode error.")
	int index = 0;

	if (pElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
		index = iPoint;
	//else if (pElement->GetMappingMode() == FbxGeometryElement::eByPolygon)
		//index = iTriangle;
	else if (pElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
		index = iTriangle * 3 + iVertex;
	else
		ThrowDxException(L"FbxGeometryElement mapping mode error.")

	if (pElement->GetReferenceMode() != FbxGeometryElement::eDirect)
		index = pElement->GetIndexArray().GetAt(index);

	return pElement->GetDirectArray().GetAt(index);
}

DirectX::XMFLOAT4X4 GFilmboxManager::ToDxMatrix(const FbxAMatrix& fbxMat)
{
	DirectX::XMFLOAT4X4 ret; 
	//FbxQuaternion q = fbxMat.GetQ();
	FbxVector4 t = fbxMat.GetT();
	FbxVector4 r = fbxMat.GetR();
	FbxVector4 s = fbxMat.GetS();
	//DirectX::XMFLOAT4 dxQ(float(q.GetAt(0)), float(q.GetAt(1)), float(q.GetAt(2)), float(q.GetAt(3)));
	//DirectX::XMMATRIX mT = DirectX::XMMatrixTranslation(float(t[0]), float(t[1]), float(t[2]));
	//DirectX::XMMATRIX mR = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&dxQ));
	//DirectX::XMMATRIX mS = DirectX::XMMatrixScaling(float(s[0]), float(s[1]), float(s[2]));
	t.Set(-t.mData[0], t.mData[1], t.mData[2]);
	r.Set(r.mData[0], -r.mData[1], -r.mData[2]);
	DirectX::XMMATRIX mT = DirectX::XMMatrixTranslation(float(t[0]), float(t[1]), float(t[2]));
	//DirectX::XMMATRIX mR = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&dxQ));
	DirectX::XMMATRIX mR = DirectX::XMMatrixRotationRollPitchYaw(float(r[0]) * MathHelper::Pi / 180, float(r[1]) * MathHelper::Pi / 180, float(r[2]) * MathHelper::Pi / 180);
	DirectX::XMMATRIX mS = DirectX::XMMatrixScaling(float(s[0]), float(s[1]), float(s[2]));
	DirectX::XMMATRIX mTrans = mS * mR * mT;
	DirectX::XMStoreFloat4x4(&ret, mTrans);
	return ret;
}

FbxAMatrix GFilmboxManager::GetGeometryTransform(FbxNode* pNode)
{
	if (!pNode)
	{
		ThrowDxException(L"Null for mesh geometry");
	}

	const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

	return FbxAMatrix(lT, lR, lS);
}