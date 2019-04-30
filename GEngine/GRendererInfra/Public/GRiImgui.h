#pragma once
#include "GRiPreInclude.h"




class GRiImgui
{

public:

	GRiImgui();
	~GRiImgui();

	virtual void BeginFrame() = 0;

	virtual void SetGUIContent(bool bShowGizmo, const float *cameraView, float *cameraProjection, float* objectLocation, float* objectRotation, float* objectScale, float& cameraSpeed) = 0;

	virtual void ShutDown() = 0;

};

