#include "stdafx.h"
#include "EGCore.h"




void __stdcall MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	GCore::GetRenderer()->MsgProc(hwnd, msg, wParam, lParam);
}

int __stdcall GetSceneObjectNum(void)
{
	return GCore::GetRenderer()->GetSceneObjectNum();
}

const char* __stdcall GetSceneObjectName(int index)
{
	return GCore::GetRenderer()->GetSceneObjectName(index);
}

void __stdcall SetSetSceneObjectsCallback(VoidFuncPointerType pSetSceneObjectsCallback)
{
	GCore::GetRenderer()->SetSetSceneObjectsCallback(pSetSceneObjectsCallback);
}

void __stdcall GetSceneObjectTransform(char* objName, float* trans)
{
	GCore::GetRenderer()->GetSceneObjectTransform(objName, trans);
}

void __stdcall SetSceneObjectTransform(char* objName, float* trans)
{
	GCore::GetRenderer()->SetSceneObjectTransform(objName, trans);
}