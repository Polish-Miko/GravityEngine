#include "stdafx.h"
#include "GRiSubmesh.h"


GRiSubmesh::GRiSubmesh()
{
}


GRiSubmesh::~GRiSubmesh()
{
}

void GRiSubmesh::SetMaterial(GRiMaterial* material)
{
	pMaterial = material;
}

GRiMaterial* GRiSubmesh::GetMaterial()
{
	return pMaterial;
}
