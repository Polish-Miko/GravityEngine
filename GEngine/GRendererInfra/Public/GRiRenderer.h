#pragma once
#include "GRiPreInclude.h"

class GRiRenderer
{
public:
	GRiRenderer();
	~GRiRenderer();

	void SetTimer(GGiGameTimer* timer);

	virtual void Update(const GGiGameTimer* gt) = 0;
	virtual void Draw(const GGiGameTimer* gt) = 0;
	virtual void Initialize(HWND OutputWindow, double width, double height) = 0;

	virtual void OnResize() = 0;

	virtual bool IsRunning() = 0;

	void SetClientWidth(int width);
	void SetClientHeight(int height);

	virtual void OnMouseDown(WPARAM btnState, int x, int y) = 0;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) = 0;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) = 0;

	void CalculateFrameStats();

protected:

	GGiGameTimer* pTimer;

	int mClientWidth = 800;
	int mClientHeight = 600;

	// Set true to use 4X MSAA.  The default is false.
	//bool      m4xMsaaState = false;    // 4X MSAA enabled
	//UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

};

