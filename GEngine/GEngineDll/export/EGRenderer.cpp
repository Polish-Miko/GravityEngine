#include "stdafx.h"
#include "EGRenderer.h"




void __stdcall InitD3D(HWND hWnd, double width, double height)
{
	GCore::GetCore().Initialize(hWnd, width, height);
}

int __stdcall Run(void)
{
	GCore::GetCore().Run();
	return 0;
}

