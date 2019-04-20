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