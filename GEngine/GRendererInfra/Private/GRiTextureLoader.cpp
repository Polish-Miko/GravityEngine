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
	if (path.rfind('.') != std::wstring::npos)
		return path.substr(path.rfind('.'));
	else
		return L"";
}

std::wstring GRiTextureLoader::GetFileName(std::wstring path)
{
	if (path.rfind('\\') != std::wstring::npos)
		return path.substr(path.rfind('\\'));
	else
		return L"";
}