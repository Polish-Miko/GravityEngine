#include "stdafx.h"
#include "GRiRenderer.h"


GRiRenderer::GRiRenderer()
{
}


GRiRenderer::~GRiRenderer()
{
}

void GRiRenderer::SetTimer(GGiGameTimer* timer)
{
	pTimer = timer;
}

void GRiRenderer::SetClientWidth(int width)
{
	mClientWidth = width;
}

void GRiRenderer::SetClientHeight(int height)
{
	mClientHeight = height;
}

void GRiRenderer::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if ((pTimer->TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);

		std::wstring windowText = L"fps: " + fpsStr + L", mspf: " + mspfStr;

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

GRiRendererFactory* GRiRenderer::GetFactory()
{
	return mFactory.get();
}


GRiFilmboxManager* GRiRenderer::GetFilmboxManager()
{
	return mFilmboxManager.get();
}

float GRiRenderer::AspectRatio() const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}

/*
void GRiRenderer::Set4xMsaaState(bool state)
{
	if (m4xMsaaState != state)
	{
		m4xMsaaState = state;

		// Recreate the swapchain and buffers with new multisample settings.
		CreateSwapChain();
		OnResize();
	}
}
*/