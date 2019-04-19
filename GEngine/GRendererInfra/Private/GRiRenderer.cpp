#include "stdafx.h"
#include "GRiRenderer.h"


GRiRenderer::GRiRenderer()
{
}


GRiRenderer::~GRiRenderer()
{
}

HWND GRiRenderer::MainWnd()const
{
	return mhMainWnd;
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

void GRiRenderer::SyncTextures(std::unordered_map<std::wstring, std::unique_ptr<GRiTexture>>& mTextures)
{
	pTextures.clear();
	std::unordered_map<std::wstring, std::unique_ptr<GRiTexture>>::iterator i;
	for (i = mTextures.begin(); i != mTextures.end(); i++)
	{
		pTextures[i->first] = i->second.get();
	}
}

void GRiRenderer::SyncMaterials(std::unordered_map<std::wstring, std::unique_ptr<GRiMaterial>>& mMaterials)
{
	pMaterials.clear();
	std::unordered_map<std::wstring, std::unique_ptr<GRiMaterial>>::iterator i;
	for (i = mMaterials.begin(); i != mMaterials.end(); i++)
	{
		pMaterials[i->first] = i->second.get();
	}
}

void GRiRenderer::SyncMeshes(std::unordered_map<std::wstring, std::unique_ptr<GRiMesh>>& mMeshes)
{
	pMeshes.clear();
	std::unordered_map<std::wstring, std::unique_ptr<GRiMesh>>::iterator i;
	for (i = mMeshes.begin(); i != mMeshes.end(); i++)
	{
		pMeshes[i->first] = i->second.get();
	}
}

void GRiRenderer::SyncSceneObjects(std::unordered_map<std::wstring, std::unique_ptr<GRiSceneObject>>& mSceneObjects, std::vector<GRiSceneObject*>* mSceneObjectLayer)
{
	pSceneObjects.clear();
	std::unordered_map<std::wstring, std::unique_ptr<GRiSceneObject>>::iterator i;
	for (i = mSceneObjects.begin(); i != mSceneObjects.end(); i++)
	{
		pSceneObjects[i->first] = i->second.get();
	}
	for (size_t layer = 0; layer != (size_t)(RenderLayer::Count); layer++)
	{
		for (auto pSObj : mSceneObjectLayer[layer])
		{
			pSceneObjectLayer[layer].push_back(pSObj);
		}
	}
}

void GRiRenderer::SyncCameras(std::vector<GRiCamera*> mCameras)
{
	pCamera = mCameras[0];
	for (auto i = 0u; i < 6; i++)
		pCubemapSampleCamera[i] = mCameras[i + 1];
}



