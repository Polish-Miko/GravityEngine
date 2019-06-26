#pragma once
#include "GRiPreInclude.h"
#include "GRiMesh.h"


enum CullState
{
	Visible,
	FrustumCulled,
	OcclusionCulled
};

class GRiSceneObject
{
public:
	GRiSceneObject() = default;
	GRiSceneObject(const GRiSceneObject& rhs) = delete;

	// Give it a name so we can look it up by name.
	std::wstring UniqueName;

	std::vector<float> GetLocation();
	std::vector<float> GetRotation();
	std::vector<float> GetScale();
	void SetLocation(float x, float y, float z);
	void SetRotation(float pitch, float yaw, float roll);
	void SetScale(float x, float y, float z);

	void MarkDirty();

	GGiFloat4x4 GetTransform();

	virtual void UpdateTransform() = 0;

	void SetTexTransform(GGiFloat4x4 texTrans);
	GGiFloat4x4 GetTexTransform();

	void SetMesh(GRiMesh* mesh);
	GRiMesh* GetMesh();

	void SetOverrideMaterial(std::wstring submeshName, GRiMaterial* mat);
	GRiMaterial* GetOverrideMaterial(std::wstring submeshName);
	std::map<std::wstring, std::wstring> GetOverrideMaterialNames();
	void ClearOverrideMaterials();

	void SetObjIndex(UINT ind);
	UINT GetObjIndex();

	GGiFloat4x4 GetPrevTransform();
	void SetPrevTransform(GGiFloat4x4 trans);
	void ResetPrevTransform();

	CullState GetCullState();
	void SetCullState(CullState cullState);

	bool IsTransformDirty();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = NUM_FRAME_RESOURCES;

protected:

	float Location[3] = { 0.0f, 0.0f, 0.0f };
	float Rotation[3] = { 0.0f, 0.0f, 0.0f };
	float Scale[3] = { 1.0f, 1.0f, 1.0f };

	GGiFloat4x4 mTransform;

	GGiFloat4x4 prevTransform;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjIndex = -1;

	//GRiMaterial* Mat = nullptr;

	std::unordered_map<std::wstring, GRiMaterial*> pOverrideMaterial;

	GRiMesh* Mesh;

	GGiFloat4x4 TexTransform;

	CullState mCullState = Visible;

	bool bTransformDirty = true;

};

