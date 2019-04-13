#pragma once
#include "GRiPreInclude.h"
#include "GRiTexture.h"
#include "GRiTextureLoader.h"

class GRiRendererFactory
{

public:
	GRiRendererFactory();
	~GRiRendererFactory();

	virtual GRiTexture* CreateTexture() = 0;

	virtual GRiTextureLoader* CreateTextureLoader() = 0;

};

