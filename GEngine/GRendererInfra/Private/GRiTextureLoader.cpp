#include "stdafx.h"
#include "GRiTextureLoader.h"


GRiTextureLoader::GRiTextureLoader()
{
}


GRiTextureLoader::~GRiTextureLoader()
{
}

std::wstring GRiTextureLoader::GetExtension(std::wstring path)
{
	if ((path.rfind('.') != std::wstring::npos) && (path.rfind('.') != (path.length() - 1)))
		return path.substr(path.rfind('.') + 1);
	else
		return L"";
}

std::wstring GRiTextureLoader::GetFileName(std::wstring path)
{
	if ((path.rfind('\\') != std::wstring::npos) && (path.rfind('\\') != (path.length() - 1)))
		return path.substr(path.rfind('\\') + 1);
	else
		return L"";
}