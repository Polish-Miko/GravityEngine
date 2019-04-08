#pragma once
#include "GCore.h"

//GRendererBox* gRendererBox;

extern "C"
{
	__declspec(dllexport) void __stdcall InitD3D(HWND hWnd, double width, double height);
}

extern "C"
{
	__declspec(dllexport) int __stdcall Run(void);
}

/*
class EGRenderer
{
public:
	EGRenderer();
	~EGRenderer();
};
*/