#pragma once
#include "GCore.h"






extern "C"
{
	__declspec(dllexport) void __stdcall InitD3D(HWND hWnd, double width, double height);
}

extern "C"
{
	__declspec(dllexport) int __stdcall Run(void);
}

extern "C"
{
	__declspec(dllexport) void __stdcall MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
}

extern "C"
{
	__declspec(dllexport) void __stdcall SetWorkDirectory(wchar_t* dir);
}

extern "C"
{
	__declspec(dllexport) int __stdcall GetSceneObjectNum(void);
}

extern "C"
{
	__declspec(dllexport) const wchar_t* __stdcall GetSceneObjectName(int index);
}

extern "C"
{
	__declspec(dllexport) void __stdcall SetSetSceneObjectsCallback(VoidFuncPointerType pSetSceneObjectsCallback);
}

extern "C"
{
	__declspec(dllexport) void __stdcall GetSceneObjectTransform(wchar_t* objName, float* trans);
}

extern "C"
{
	__declspec(dllexport) void __stdcall SetSceneObjectTransform(wchar_t* objName, float* trans);
}

extern "C"
{
	__declspec(dllexport) bool __stdcall GetTextureSrgb(wchar_t* txtName);
}

extern "C"
{
	__declspec(dllexport) void __stdcall SetTextureSrgb(wchar_t* txtName, bool bSrgb);
}

/*
class EGCore
{
public:
	EGCore();
	~EGCore();
};
*/