#pragma once
#include "GCore.h"




extern "C"
{
	__declspec(dllexport) void __stdcall MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
}

extern "C"
{
	__declspec(dllexport) int __stdcall GetSceneObjectNum(void);
}

extern "C"
{
	__declspec(dllexport) const char* __stdcall GetSceneObjectName(int index);
}

extern "C"
{
	__declspec(dllexport) void __stdcall SetSetSceneObjectsCallback(VoidFuncPointerType pSetSceneObjectsCallback);
}

extern "C"
{
	__declspec(dllexport) void __stdcall GetSceneObjectTransform(char* objName, float* trans);
}

extern "C"
{
	__declspec(dllexport) void __stdcall SetSceneObjectTransform(char* objName, float* trans);
}

/*
class EGCore
{
public:
	EGCore();
	~EGCore();
};
*/