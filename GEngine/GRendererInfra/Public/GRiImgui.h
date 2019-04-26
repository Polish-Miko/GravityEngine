#pragma once
#include "GRiPreInclude.h"




class GRiImgui
{
public:
	GRiImgui();
	~GRiImgui();

	virtual void BeginFrame() = 0;
	virtual void SetGUIContent() = 0;
	virtual void ShutDown() = 0;

};

