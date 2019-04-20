#pragma once
#include "GRiPreInclude.h"
#include "GRiRendererFactory.h"
#include "GRiFilmboxManager.h"
#include "GRiCamera.h"

class GRiRenderer
{
public:
	GRiRenderer();
	~GRiRenderer();

	void SetTimer(GGiGameTimer* timer);

	virtual void Update(const GGiGameTimer* gt) = 0;
	virtual void Draw(const GGiGameTimer* gt) = 0;
	virtual void Initialize(HWND OutputWindow, double width, double height) = 0;
	virtual void PreInitialize(HWND OutputWindow, double width, double height) = 0;

	virtual void OnResize() = 0;

	virtual bool IsRunning() = 0;

	virtual float AspectRatio() const;
	HWND MainWnd() const;

	void SetClientWidth(int width);
	void SetClientHeight(int height);

	void CalculateFrameStats();

	virtual void CreateRendererFactory() = 0;
	GRiRendererFactory* GetFactory();

	virtual void CreateFilmboxManager() = 0;
	GRiFilmboxManager* GetFilmboxManager();

	virtual void SyncTextures(std::unordered_map<std::wstring, std::unique_ptr<GRiTexture>>& mTextures);
	virtual void SyncMaterials(std::unordered_map<std::wstring, std::unique_ptr<GRiMaterial>>& mMaterials);
	virtual void SyncMeshes(std::unordered_map<std::wstring, std::unique_ptr<GRiMesh>>& mMeshes);
	virtual void SyncSceneObjects(std::unordered_map<std::wstring, std::unique_ptr<GRiSceneObject>>& mSceneObjects, std::vector<GRiSceneObject*>* mSceneObjectLayer);
	virtual void SyncCameras(std::vector<GRiCamera*> mCameras);

	std::unordered_map<std::wstring, GRiTexture*> pTextures;
	std::unordered_map<std::wstring, GRiMaterial*> pMaterials;
	std::unordered_map<std::wstring, GRiMesh*> pMeshes;
	std::unordered_map<std::wstring, GRiSceneObject*> pSceneObjects;
	std::vector<GRiSceneObject*> pSceneObjectLayer[(int)RenderLayer::Count];

	virtual void RegisterTexture(GRiTexture* text) = 0;
	std::vector<int> mTexturePoolFreeIndex;

	GRiCamera* pCamera = nullptr;
	GRiCamera* pCubemapSampleCamera[6];

protected:

	HWND mhMainWnd = nullptr; // main window handle

	GGiGameTimer* pTimer;

	int mClientWidth = 800;
	int mClientHeight = 600;

	std::unique_ptr<GRiRendererFactory> mFactory;

	std::unique_ptr<GRiFilmboxManager> mFilmboxManager;

};


