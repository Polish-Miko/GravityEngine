#include "stdafx.h"
#include "GRiMaterial.h"


GRiMaterial::GRiMaterial()
{
}


GRiMaterial::~GRiMaterial()
{
}



void GRiMaterial::SetScale(float x, float y)
{
	MaterialScale[0] = x;
	MaterialScale[1] = y;
	MarkDirty();
}

std::vector<float> GRiMaterial::GetScale()
{
	std::vector<float> ret;
	ret.push_back(MaterialScale[0]);
	ret.push_back(MaterialScale[1]);
	return ret;
}

float GRiMaterial::GetScaleX()
{
	return MaterialScale[0];
}

float GRiMaterial::GetScaleY()
{
	return MaterialScale[1];
}

void GRiMaterial::MarkDirty()
{
	NumFramesDirty = NUM_FRAME_RESOURCES;
}

size_t GRiMaterial::GetTextureNum()
{
	return pTextures.size();
}

size_t GRiMaterial::GetScalarNum()
{
	return ScalarParams.size();
}

size_t GRiMaterial::GetVectorNum()
{
	return VectorParams.size();
}

/*
int GRiMaterial::GetTextureIndex(int index)
{
	return pTextures[index]->texIndex;
}
*/

float GRiMaterial::GetScalar(int index)
{
	return ScalarParams[index];
}

GGiVector4 GRiMaterial::GetVector(int index)
{
	return VectorParams[index];
}

void GRiMaterial::AddTexture(GRiTexture* tex)
{
	pTextures.push_back(tex->UniqueFileName);
	MarkDirty();
}

void GRiMaterial::AddTexture(std::wstring tex)
{
	pTextures.push_back(tex);
	MarkDirty();
}

void GRiMaterial::AddScalar(float scalar)
{
	ScalarParams.push_back(scalar);
	MarkDirty();
}

void GRiMaterial::AddVector(GGiVector4 vector)
{
	VectorParams.push_back(vector);
	MarkDirty();
}

/*
std::wstring GRiMaterial::GetTextureUniqueName(int index)
{
	return pTextures[index]->UniqueFileName;
}

std::wstring* GRiMaterial::GetTextureUniqueNamePtr(int index)
{
	return &(pTextures[index]->UniqueFileName);
}
*/

void GRiMaterial::SetTextureByIndex(int index, GRiTexture* tex)
{
	pTextures[index] = tex->UniqueFileName;
	MarkDirty();
}

std::wstring& GRiMaterial::GetTextureUniqueNameByIndex(int index)
{
	return pTextures[index];
}

