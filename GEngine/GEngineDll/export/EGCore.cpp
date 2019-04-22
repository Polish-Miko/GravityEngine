#include "stdafx.h"
#include "EGCore.h"





void __stdcall InitD3D(HWND hWnd, double width, double height)
{
	GCore::GetCore().Initialize(hWnd, width, height);
}

int __stdcall Run(void)
{
	GCore::GetCore().Run();
	return 0;
}

void __stdcall MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//GDxRenderer::GetRenderer().MsgProc(hwnd, msg, wParam, lParam);
	GCore::GetCore().MsgProc(hwnd, msg, wParam, lParam);
}

void __stdcall SetWorkDirectory(wchar_t* dir)
{
	GCore::GetCore().SetWorkDirectory(dir);
}

int __stdcall GetSceneObjectNum(void)
{
	return GCore::GetCore().GetSceneObjectNum();
}

const wchar_t* __stdcall GetSceneObjectName(int index)
{
	return GCore::GetCore().GetSceneObjectName(index);
}

void __stdcall SetSetSceneObjectsCallback(VoidFuncPointerType pSetSceneObjectsCallback)
{
	GCore::GetCore().SetSetSceneObjectsCallback(pSetSceneObjectsCallback);
}

void __stdcall GetSceneObjectTransform(wchar_t* objName, float* trans)
{
	GCore::GetCore().GetSceneObjectTransform(objName, trans);
}

void __stdcall SetSceneObjectTransform(wchar_t* objName, float* trans)
{
	GCore::GetCore().SetSceneObjectTransform(objName, trans);
}

bool __stdcall GetTextureSrgb(wchar_t* txtName)
{
	return GCore::GetCore().GetTextureSrgb(txtName);
}

void __stdcall SetTextureSrgb(wchar_t* txtName, bool bSrgb)
{
	GCore::GetCore().SetTextureSrgb(txtName, bSrgb);
}

void __stdcall SetProjectName(wchar_t* projName)
{
	return GCore::GetCore().SetProjectName(projName);
}

void __stdcall SaveProject()
{
	return GCore::GetCore().SaveProject();
}

void __stdcall CreateMaterial(wchar_t* cUniqueName)
{
	GCore::GetCore().CreateMaterial(cUniqueName);
}

void __stdcall GetMaterialScale(wchar_t* matUniqueName, float* scale)
{
	GCore::GetCore().GetMaterialScale(matUniqueName, scale);
}

void __stdcall SetMaterialScale(wchar_t* matUniqueName, float* scale)
{
	GCore::GetCore().SetMaterialScale(matUniqueName, scale);
}

const wchar_t* __stdcall GetMaterialTextureUniqueName(wchar_t* matUniqueName, int index)
{
	return GCore::GetCore().GetMaterialTextureUniqueName(matUniqueName, index);
}

bool __stdcall SetMaterialTexture(wchar_t* matUniqueName, int index, wchar_t* texUniqueName)
{
	return GCore::GetCore().SetMaterialTexture(matUniqueName, index, texUniqueName);
}

void __stdcall SetMaterialTextureToDefaultValue(wchar_t* matUniqueName, int index)
{
	GCore::GetCore().SetMaterialTextureToDefaultValue(matUniqueName, index);
}

void __stdcall RenameMaterial(wchar_t* oldUniqueName, wchar_t* newUniqueName)
{
	GCore::GetCore().RenameMaterial(oldUniqueName, newUniqueName);
}





