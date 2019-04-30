#pragma once
#include "GDxInclude.h"




typedef void(__stdcall * VoidFuncPointerType)(void);
typedef void(__stdcall * VoidWstringFuncPointerType)(const wchar_t*);



class GGuiCallback
{

public:

	GGuiCallback();
	~GGuiCallback();

	void SetSelectSceneObjectCallback(VoidWstringFuncPointerType callback);

	void SelectSceneObjectCallback(const wchar_t* sceneObjectName);

	void SetRefreshSceneObjectTransformCallback(VoidFuncPointerType pRefreshSceneObjectTransformCallback);

	void RefreshSceneObjectTransformCallback();

private:

	VoidWstringFuncPointerType mSelectSceneObjectCallback;

	VoidFuncPointerType mRefreshSceneObjectTransformCallback;

};

