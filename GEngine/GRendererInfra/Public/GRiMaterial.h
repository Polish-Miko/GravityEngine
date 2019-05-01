#pragma once
#include "GRiPreInclude.h"
#include "GRiTexture.h"

#define MATERIAL_MAX_TEXTURE_NUM 16
#define MATERIAL_MAX_SCALAR_NUM 16
#define MATERIAL_MAX_VECTOR_NUM 16

class GRiMaterial
{
public:
	GRiMaterial();
	~GRiMaterial();
	  
	// Unique material name for lookup.
	std::wstring UniqueName;

	// Name displayed in the engine.
	std::wstring Name;

	// Dirty flag indicating the material has changed and we need to update the constant buffer.
	// Because we have a material constant buffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify a material we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = NUM_FRAME_RESOURCES;

	// Index into constant buffer corresponding to this material.
	int MatIndex = -1;

	void SetScale(float x, float y);

	std::vector<float> GetScale();

	float GetScaleX();

	float GetScaleY();

	void MarkDirty();

	size_t GetTextureNum();

	size_t GetScalarNum();

	size_t GetVectorNum();

	//int GetTextureIndex(int index);

	float GetScalar(int index);

	GGiFloat4 GetVector(int index);

	void AddTexture(GRiTexture* tex);

	void AddTexture(std::wstring tex);

	void AddScalar(float scalar);

	void AddVector(GGiFloat4 vector);

	//std::wstring GetTextureUniqueName(int index);

	//std::wstring* GetTextureUniqueNamePtr(int index);

	void SetTextureByIndex(int index, GRiTexture* tex);

	std::wstring& GetTextureUniqueNameByIndex(int index);

protected:

	float MaterialScale[2] = { 1.0f,1.0f };

	// Textures used in this material.
	//std::vector<GRiTexture*> pTextures;
	std::vector<std::wstring> pTextures;

	// Material constants.
	std::vector<float> ScalarParams;
	std::vector<GGiFloat4> VectorParams;

};

