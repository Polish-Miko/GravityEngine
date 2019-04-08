#include "stdafx.h"
#include "EGRenderer.h"




void __stdcall InitD3D(HWND hWnd, double width, double height)
{
	
	GCore::GetRenderer()->Initialize(hWnd, width, height);
}

int __stdcall Run(void)
{
	GCore::GetRenderer()->Run();
	return 0;
}

