#pragma once
#include "GRiTexture.h"

class GRiTextureLoader
{

public:

	GRiTextureLoader();
	~GRiTextureLoader();

	virtual GRiTexture* LoadTexture(std::wstring workdir, std::wstring path, bool bSrgb) = 0;

	//virtual void LoadTexture(GRiTexture* pTexture, std::wstring workdir, std::wstring path, bool bSrgb) = 0;

protected:

	std::wstring GetExtension(std::wstring path);

	std::wstring GetFileName(std::wstring path);

};

