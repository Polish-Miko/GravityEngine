#include "stdafx.h"
#include "GRiSceneObject.h"


/*
GRiSceneObject::GRiSceneObject()
{
}


GRiSceneObject::~GRiSceneObject()
{
}
*/

void GRiSceneObject::MarkDirty()
{
	NumFramesDirty = NUM_FRAME_RESOURCES;
}

std::vector<float> GRiSceneObject::GetLocation()
{
	std::vector<float> ret = { Location[0], Location[1], Location[2] };
	return ret;
}

std::vector<float> GRiSceneObject::GetRotation()
{
	std::vector<float> ret = { Rotation[0], Rotation[1], Rotation[2] };
	return ret;
}

std::vector<float> GRiSceneObject::GetScale()
{
	std::vector<float> ret = { Scale[0], Scale[1], Scale[2] };
	return ret;
}

void GRiSceneObject::SetLocation(float x, float y, float z)
{
	Location[0] = x;
	Location[1] = y;
	Location[2] = z;
	MarkDirty();
	bTransformDirty = true;
}

void GRiSceneObject::SetRotation(float pitch, float yaw, float roll)
{
	Rotation[0] = pitch;
	Rotation[1] = yaw;
	Rotation[2] = roll;
	MarkDirty();
	bTransformDirty = true;
}

void GRiSceneObject::SetScale(float x, float y, float z)
{
	Scale[0] = x;
	Scale[1] = y;
	Scale[2] = z;
	MarkDirty();
	bTransformDirty = true;
}

void GRiSceneObject::SetTexTransform(GGiFloat4x4* texTrans)
{
	std::shared_ptr<GGiFloat4x4> temp(texTrans);
	TexTransform = temp;
	//TexTransform = std::make_shared<GGiFloat4x4>(*texTrans);
	MarkDirty();
}

std::shared_ptr<GGiFloat4x4> GRiSceneObject::GetTexTransform()
{
	return TexTransform;
}

void GRiSceneObject::SetMesh(GRiMesh* mesh)
{
	Mesh = mesh;
	MarkDirty();
}

GRiMesh* GRiSceneObject::GetMesh()
{
	return Mesh;
}

/*
void GRiSceneObject::SetMaterial(GRiMaterial* mat)
{
	Mat = mat;
	MarkDirty();
}

GRiMaterial* GRiSceneObject::GetMaterial()
{
	return Mat;
}
*/

void GRiSceneObject::SetObjIndex(UINT ind)
{
	ObjIndex = ind;
	MarkDirty();
}

UINT GRiSceneObject::GetObjIndex()
{
	return ObjIndex;
}

std::shared_ptr<GGiFloat4x4> GRiSceneObject::GetTransform()
{
	if (bTransformDirty)
		ThrowGGiException("Trying to read dirty transform data. Please make sure UpdateTransform() is called before access transform data.");

	//return mTransform.get();
	return mTransform;
}

std::shared_ptr<GGiFloat4x4> GRiSceneObject::GetPrevTransform()
{
	return prevTransform;
}

void GRiSceneObject::SetPrevTransform(GGiFloat4x4* trans)
{
	std::shared_ptr<GGiFloat4x4> temp(trans);
	prevTransform = temp;

	MarkDirty();
}

void GRiSceneObject::SetPrevTransform(std::shared_ptr<GGiFloat4x4> trans)
{
	prevTransform = trans;

	MarkDirty();
}

void GRiSceneObject::ResetPrevTransform()
{
	prevTransform = mTransform;
	//std::shared_ptr<GGiFloat4x4> temp(GetTransform());
	//prevTransform = temp;

	MarkDirty();
}

CullState GRiSceneObject::GetCullState()
{
	return mCullState;
}

void GRiSceneObject::SetCullState(CullState cullState)
{
	mCullState = cullState;
	//there's no need to mark dirty.
}

bool GRiSceneObject::IsTransformDirty()
{
	return bTransformDirty;
}



