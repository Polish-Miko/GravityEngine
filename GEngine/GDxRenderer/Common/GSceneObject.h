#pragma once

#include "GMesh.h"
//#include "Material.h"
#include "GMaterial.h"

class GSceneObject
{
public:
	GSceneObject() = default;
	GSceneObject(const GSceneObject& rhs) = delete;

	// Give it a name so we can look it up by name.
	std::string Name;

	// DrawIndexedInstanced parameters.
	//UINT IndexCount = 0; 
	//UINT StartIndexLocation = 0;
	//int BaseVertexLocation = 0;

	//std::string SubmeshName;

	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	//XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT3 GetLocation();
	XMFLOAT3 GetRotation();
	XMFLOAT3 GetScale();
	void SetLocation(XMFLOAT3 loc);
	void SetRotation(XMFLOAT3 rot);
	void SetScale(XMFLOAT3 scale);
	void SetLocation(float x, float y, float z);
	void SetRotation(float pitch, float yaw, float roll);
	void SetScale(float x, float y, float z);

	void MarkDirty();

	XMFLOAT4X4 GetTransform();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = NUM_FRAME_RESOURCES;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	GRiMaterial* Mat = nullptr;
	//Material* LegMat = nullptr;

	//std::shared_ptr<GMesh> Mesh;
	GRiMesh* Mesh;

	//GSubmesh GetSubmesh();

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

private:

	XMFLOAT3 Location = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

};

