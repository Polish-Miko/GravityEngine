#pragma once
#include "GRiPreInclude.h"
#include "GRiTexture.h"
#include "GRiTextureLoader.h"
#include "GRiMaterial.h"

class GRiRendererFactory
{

public:
	GRiRendererFactory();
	~GRiRendererFactory();

	virtual GRiTexture* CreateTexture() = 0;

	virtual GRiTextureLoader* CreateTextureLoader() = 0;

	virtual GRiMaterial* CreateMaterial() = 0;

};

