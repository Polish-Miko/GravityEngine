#include "stdafx.h"
#include "GSceneObject.h"

/*
GMesh::GMesh()
{
}


GMesh::~GMesh()
{
}

GSubmesh GSceneObject::GetSubmesh()
{
	return Mesh->Submeshes[SubmeshName];
}

*/

XMFLOAT4X4 GSceneObject::GetTransform()
{
	XMFLOAT4X4 trans;
	XMMATRIX matLoc = XMMatrixTranslation(GetLocation().x, GetLocation().y, GetLocation().z);
	XMMATRIX matRot = XMMatrixRotationRollPitchYaw(GetRotation().x, GetRotation().y, GetRotation().z);
	XMMATRIX matScale = XMMatrixScaling(GetScale().x, GetScale().y, GetScale().z);
	XMMATRIX matTrans = matScale * matRot * matLoc;
	XMStoreFloat4x4(&trans, matTrans);
	return trans;
}

XMFLOAT3 GSceneObject::GetLocation()
{
	return Location;
}

XMFLOAT3 GSceneObject::GetRotation()
{
	return Rotation;
}

XMFLOAT3 GSceneObject::GetScale()
{
	return Scale;
}

void GSceneObject::SetLocation(XMFLOAT3 loc)
{
	Location = loc;
	MarkDirty();
}

void GSceneObject::SetRotation(XMFLOAT3 rot)
{
	Rotation = rot;
	MarkDirty();
}

void GSceneObject::SetScale(XMFLOAT3 scale)
{
	Scale = scale;
	MarkDirty();
}

void GSceneObject::SetLocation(float x, float y, float z)
{
	Location = XMFLOAT3(x, y, z);
	MarkDirty();
}

void GSceneObject::SetRotation(float pitch, float yaw, float roll)
{
	Rotation = XMFLOAT3(pitch, yaw, roll);
	MarkDirty();
}

void GSceneObject::SetScale(float x, float y, float z)
{
	Scale = XMFLOAT3(x, y, z);
	MarkDirty();
}

void GSceneObject::MarkDirty()
{
	NumFramesDirty = NUM_FRAME_RESOURCES;
}