#pragma once

#include "GUtilInclude.h"

class GShaderManager
{
public:
	GShaderManager(){}
	~GShaderManager(){}

	static D3D12_SHADER_BYTECODE LoadShader(std::wstring shaderCsoFile);

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);
};

