#pragma once
#include "GDxPreInclude.h"



class GDxImgui : public GRiImgui
{

public:

	GDxImgui();
	~GDxImgui();

	void Initialize(HWND handleWnd, ID3D12Device* pDevice, int NumFrameResources, ID3D12DescriptorHeap* pDescriptorHeap);

	virtual void BeginFrame() override;
	virtual void SetGUIContent() override;
	void Render(ID3D12GraphicsCommandList* cmdList);
	virtual void ShutDown() override;

private:

};

