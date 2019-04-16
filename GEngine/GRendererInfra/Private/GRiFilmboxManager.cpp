#include "stdafx.h"
#include "GRiFilmboxManager.h"


GRiFilmboxManager::GRiFilmboxManager()
{
}


GRiFilmboxManager::~GRiFilmboxManager()
{
}


bool GRiFilmboxManager::ImportFbxFile_Mesh(std::wstring* FileName, std::vector<GRiMeshData> MeshDataList)
{
	const wchar_t *source_path_WC = FileName->c_str();

	// convert to UTF8
	char *fileName_UTF8;
	FbxWCToUTF8(source_path_WC, fileName_UTF8);
	FbxString lSourceFileName(fileName_UTF8);
	FbxAutoFreePtr<char> lsp(fileName_UTF8);

}
