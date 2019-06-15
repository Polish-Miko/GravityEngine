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

void GDxSceneObject::UpdateTransform()
{
	/*
	GDxFloat4x4* matLoc = new GDxFloat4x4();
	matLoc->SetByTranslation(Location[0], Location[1], Location[2]);
	GDxFloat4x4* matRot = new GDxFloat4x4();
	matRot->SetByRotationPitchYawRoll(Rotation[0] * GGiEngineUtil::PI / 180.0f, Rotation[1] * GGiEngineUtil::PI / 180.0f, Rotation[2] * GGiEngineUtil::PI / 180.0f);
	GDxFloat4x4* matScale = new GDxFloat4x4();
	matScale->SetByScale(Scale[0], Scale[1], Scale[2]);
	//GGiFloat4x4* trans = &((*matScale) * (*matRot) * (*matLoc));
	GGiFloat4x4* temp = &((*matScale) * (*matRot));
	GGiFloat4x4* trans = &((*temp) * (*matLoc));
	delete temp;
	delete matScale;
	delete matRot;
	delete matLoc;
	*/
	auto t = DirectX::XMMatrixTranslation(Location[0], Location[1], Location[2]);
	auto r = DirectX::XMMatrixRotationRollPitchYaw(Rotation[0] * GGiEngineUtil::PI / 180.0f, Rotation[1] * GGiEngineUtil::PI / 180.0f, Rotation[2] * GGiEngineUtil::PI / 180.0f);
	auto s = DirectX::XMMatrixScaling(Scale[0], Scale[1], Scale[2]);
	auto transMat = DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(s, r), t);
	DirectX::XMFLOAT4X4 transfloat4x4;
	DirectX::XMStoreFloat4x4(&transfloat4x4, transMat);
	GDxFloat4x4* trans = new GDxFloat4x4();
	trans->SetValue(transfloat4x4);
	std::shared_ptr<GGiFloat4x4> temp(trans);
	mTransform = temp;
	bTransformDirty = false;
}


