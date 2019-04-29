#include "stdafx.h"
#include "GGuiCallback.h"


GGuiCallback::GGuiCallback()
{
}


GGuiCallback::~GGuiCallback()
{
}

void GGuiCallback::SetSelectSceneObjectCallback(VoidWstringFuncPointerType callback)
{
	mSelectSceneObjectCallback = callback;
}

void GGuiCallback::SelectSceneObjectCallback(const wchar_t* sceneObjectName)
{
	mSelectSceneObjectCallback(sceneObjectName);
}






