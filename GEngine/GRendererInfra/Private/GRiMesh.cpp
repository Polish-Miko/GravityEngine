#include "stdafx.h"
#include "GRiMesh.h"


/*
GRiMesh::GRiMesh()
{
}


GRiMesh::~GRiMesh()
{
}
*/


/*
void GRiMesh::MarkDirty()
{
	NumFramesDirty = NUM_FRAME_RESOURCES;
}

void GRiMesh::SetMaterial(std::wstring submeshName, GRiMaterial* mat)
{
	if (Submeshes.find(submeshName) != Submeshes.end())
	{
		Submeshes[submeshName].SetMaterial(mat);
		MarkDirty();
	}
	else
		ThrowGGiException(L"Submesh \"" + submeshName + L"\" Not Found.");
}

GRiMaterial* GRiMesh::GetMaterial(std::wstring submeshName)
{
	if (Submeshes.find(submeshName) != Submeshes.end())
		return Submeshes[submeshName].GetMaterial();
	else
		return nullptr;
}
*/

int GRiMesh::GetSdfResolution()
{
	return SdfResolution;
}

void GRiMesh::SetSdfResolution(int res)
{
	SdfResolution = res;
}

std::shared_ptr<std::vector<float>> GRiMesh::GetSdf()
{
	return SignedDistanceField;
}

void GRiMesh::InitializeSdf(std::vector<float>& sdf)
{
	SignedDistanceField = std::make_shared<std::vector<float>>(sdf);
}




