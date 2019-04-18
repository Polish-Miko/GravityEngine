#include "stdafx.h"
#include "GDxShaderManager.h"

/*
GShaderManager::GShaderManager()
{
}


GShaderManager::~GShaderManager()
{
}
*/

Microsoft::WRL::ComPtr<ID3DBlob> GDxShaderManager::CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	ThrowIfFailed(hr);

	return byteCode;
}

D3D12_SHADER_BYTECODE GDxShaderManager::LoadShader(std::wstring shaderCsoFile)
{
	ID3DBlob* shaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(shaderCsoFile.c_str(), &shaderBlob));
	D3D12_SHADER_BYTECODE shaderBytecode = {};
	shaderBytecode.BytecodeLength = shaderBlob->GetBufferSize();
	shaderBytecode.pShaderBytecode = shaderBlob->GetBufferPointer();
	return shaderBytecode;
}