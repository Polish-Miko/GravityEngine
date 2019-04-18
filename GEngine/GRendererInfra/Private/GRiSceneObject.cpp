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
}

void GRiSceneObject::SetRotation(float pitch, float yaw, float roll)
{
	Rotation[0] = pitch;
	Rotation[1] = yaw;
	Rotation[2] = roll;
	MarkDirty();
}

void GRiSceneObject::SetScale(float x, float y, float z)
{
	Scale[0] = x;
	Scale[1] = y;
	Scale[2] = z;
	MarkDirty();
}

void GRiSceneObject::MarkDirty()
{
	NumFramesDirty = NUM_FRAME_RESOURCES;
}