#pragma once
#include "GRiPreInclude.h"
#include "GRiTexture.h"
#include "GRiTextureLoader.h"
#include "GRiMaterial.h"
#include "GGiInclude.h"
#include "GRiMesh.h"
#include "GRiMeshData.h"
#include "GRiGeometryGenerator.h"
#include "GRiSceneObject.h"

class GRiRendererFactory
{

public:
	GRiRendererFactory();
	~GRiRendererFactory();

	virtual GRiTexture* CreateTexture() = 0;

	virtual GRiTextureLoader* CreateTextureLoader() = 0;

	virtual GRiMaterial* CreateMaterial() = 0;

	virtual GGiFloat4* CreateFloat4() = 0;

	virtual GGiFloat4* CreateFloat4(float x, float y, float z, float w) = 0;

	virtual GGiFloat4x4* CreateFloat4x4() = 0;

	virtual GRiMesh* CreateMesh(std::vector<GRiMeshData> meshData) = 0;

	virtual GRiGeometryGenerator* CreateGeometryGenerator() = 0;

	virtual GRiSceneObject* CreateSceneObject() = 0;

};

