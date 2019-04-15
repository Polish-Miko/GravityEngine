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

	// Textures used in this material.
	std::vector<GRiTexture*> pTextures;

	// Material constants.
	std::vector<float> ScalarParams;
	std::vector<GGiFloat4> VectorParams;

	GGiFloat4x4* MatTransform;// = MathHelper::Identity4x4();

	// Dirty flag indicating the material has changed and we need to update the constant buffer.
	// Because we have a material constant buffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify a material we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = NUM_FRAME_RESOURCES;

	// Index into constant buffer corresponding to this material.
	int MatIndex = -1;
};

