#pragma once
#include "GRiPreInclude.h"
#include "GRiMaterial.h"


class GRiSubmesh
{

public:

	GRiSubmesh();
	~GRiSubmesh();

	std::wstring Name;

	UINT IndexCount = 0;

	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

	void SetMaterial(GRiMaterial* material);

	GRiMaterial* GetMaterial();

private:

	GRiMaterial* pMaterial = nullptr;

};

