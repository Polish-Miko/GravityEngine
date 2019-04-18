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

GGiFloat4x4* GDxSceneObject::GetTransform()
{
	GDxFloat4x4* matLoc = new GDxFloat4x4();
	matLoc->SetByTranslation(Location[0], Location[1], Location[2]);
	GDxFloat4x4* matRot = new GDxFloat4x4();
	matLoc->SetByTranslation(Rotation[0], Rotation[1], Rotation[2]);
	GDxFloat4x4* matScale = new GDxFloat4x4();
	matLoc->SetByTranslation(Scale[0], Scale[1], Scale[2]);
	GGiFloat4x4* trans = &((*matScale) * (*matRot) * (*matLoc));
	return trans;
}
