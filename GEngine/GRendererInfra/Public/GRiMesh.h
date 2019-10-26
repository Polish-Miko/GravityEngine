#pragma once
#include "GRiPreInclude.h"
#include "GRiSubmesh.h"
#include "GRiBoundingBox.h"


class GRiMesh
{
public:

	GRiMesh() = default;
	//GRiMesh(const GRiMesh& rhs) = delete;
	~GRiMesh() = default;

	// Unique name for hash table lookup.
	std::wstring UniqueName;

	std::wstring Name;

	bool bIsSkeletalMesh = false;

	virtual void AllowDynamicCast() {};

	GRiBoundingBox bounds;

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = NUM_FRAME_RESOURCES;

	/*
	void MarkDirty();

	void SetMaterial(std::wstring submeshName, GRiMaterial* mat);

	GRiMaterial* GetMaterial(std::wstring submeshName);

private:
	*/

	std::unordered_map<std::wstring, GRiSubmesh> Submeshes;

	int GetSdfResolution();
	void SetSdfResolution(int res);

	std::shared_ptr<std::vector<float>> GetSdf();
	void InitializeSdf(std::vector<float>& sdf);

	int mSdfIndex = 0;

protected:

	std::shared_ptr<std::vector<float>> SignedDistanceField;

	int SdfResolution = 32;

};

