#include "stdafx.h"
#include "EGRenderer.h"




void __stdcall InitD3D(HWND hWnd, double width, double height)
{
	
	GDxRenderer::GetRenderer().Initialize(hWnd, width, height);
}

int __stdcall Run(void)
{
	GDxRenderer::GetRenderer().Run();
	return 0;
}

