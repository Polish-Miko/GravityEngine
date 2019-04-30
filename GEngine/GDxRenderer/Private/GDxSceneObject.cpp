#include "stdafx.h"
#include "GDxSceneObject.h"
#include "GDxFloat4x4.h"


/*
GDxSceneObject::GDxSceneObject()
{
}


GDxSceneObject::~GDxSceneObject()
{
}
*/

D3D12_PRIMITIVE_TOPOLOGY GDxSceneObject::GetPrimitiveTopology()
{
	return PrimitiveType;
}

void GDxSceneObject::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topo)
{
	PrimitiveType = topo;
	MarkDirty();
}

GGiFloat4x4* GDxSceneObject::GetTransform()
{
	GDxFloat4x4* matLoc = new GDxFloat4x4();
	matLoc->SetByTranslation(Location[0], Location[1], Location[2]);
	GDxFloat4x4* matRot = new GDxFloat4x4();
	matRot->SetByRotationPitchYawRoll(Rotation[0] * GGiEngineUtil::PI / 180.0f, Rotation[1] * GGiEngineUtil::PI / 180.0f, Rotation[2] * GGiEngineUtil::PI / 180.0f);
	GDxFloat4x4* matScale = new GDxFloat4x4();
	matScale->SetByScale(Scale[0], Scale[1], Scale[2]);
	GGiFloat4x4* trans = &((*matLoc) * (*matRot) * (*matScale));
	//GGiFloat4x4* trans = &((*matScale) * (*matRot) * (*matLoc));
	return trans;
}


