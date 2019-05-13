#pragma once
#include "GDxPreInclude.h"



class GDxImgui : public GRiImgui
{

public:

	GDxImgui();
	~GDxImgui();

	void Initialize(HWND handleWnd, ID3D12Device* pDevice, int NumFrameResources, ID3D12DescriptorHeap* pDescriptorHeap);

	virtual void BeginFrame() override;

	virtual void SetGUIContent(
		bool bShowGizmo, 
		const float *cameraView,
		float *cameraProjection,
		float* objectLocation,
		float* objectRotation,
		float* objectScale,
		float& cameraSpeed,
		std::vector<ProfileData> gpuProfiles,
		int clientWidth,
		int clientHeight
	) override;

	void Render(ID3D12GraphicsCommandList* cmdList);

	virtual void ShutDown() override;
	
private:
	
	void Manipulation(bool bShowGizmo, const float *cameraView, float *cameraProjection, float* objectLocation, float* objectRotation, float* objectScale, float& cameraSpeed);

	static ImU8 mCameraSpeedUpperBound;
	static ImU8 mCameraSpeedLowerBound;
	static float mInitialCameraSpeed;

};

