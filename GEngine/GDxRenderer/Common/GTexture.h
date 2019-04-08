#pragma once

#include "GUtilInclude.h"

class GTexture
{

public:

	GTexture();
	~GTexture();

	std::string Name;
	std::wstring Filename;

	int descriptorHeapIndex;

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
};

