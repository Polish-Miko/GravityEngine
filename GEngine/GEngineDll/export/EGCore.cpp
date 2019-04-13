#include "stdafx.h"
#include "EGCore.h"




void __stdcall MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	GDxRenderer::GetRenderer().MsgProc(hwnd, msg, wParam, lParam);
}

int __stdcall GetSceneObjectNum(void)
{
	return GDxRenderer::GetRenderer().GetSceneObjectNum();
}

const char* __stdcall GetSceneObjectName(int index)
{
	return GDxRenderer::GetRenderer().GetSceneObjectName(index);
}

void __stdcall SetSetSceneObjectsCallback(VoidFuncPointerType pSetSceneObjectsCallback)
{
	GDxRenderer::GetRenderer().SetSetSceneObjectsCallback(pSetSceneObjectsCallback);
}

void __stdcall GetSceneObjectTransform(char* objName, float* trans)
{
	GDxRenderer::GetRenderer().GetSceneObjectTransform(objName, trans);
}

void __stdcall SetSceneObjectTransform(char* objName, float* trans)
{
	GDxRenderer::GetRenderer().SetSceneObjectTransform(objName, trans);
}