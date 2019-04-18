#pragma once
#include "GRiPreInclude.h"
#include "GRiRendererFactory.h"
#include "GRiFilmboxManager.h"

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

	void SetClientWidth(int width);
	void SetClientHeight(int height);

	virtual void OnMouseDown(WPARAM btnState, int x, int y) = 0;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) = 0;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) = 0;

	void CalculateFrameStats();

	virtual void CreateRendererFactory() = 0;
	GRiRendererFactory* GetFactory();

	virtual void CreateFilmboxManager() = 0;
	GRiFilmboxManager* GetFilmboxManager();

	virtual void SyncTextures(std::unordered_map<std::wstring, std::unique_ptr<GRiTexture>>& mTextures) = 0;
	virtual void SyncMaterials(std::unordered_map<std::wstring, std::unique_ptr<GRiMaterial>>& mMaterials) = 0;
	virtual void SyncMeshes(std::unordered_map<std::wstring, std::unique_ptr<GRiMesh>>& mMeshes) = 0;
	virtual void SyncSceneObjects(std::unordered_map<std::wstring, std::unique_ptr<GRiSceneObject>>& mSceneObjects, std::vector<GRiSceneObject*>* mSceneObjectLayer) = 0;

	std::unordered_map<std::wstring, GRiTexture*> pTextures;
	std::unordered_map<std::wstring, GRiMaterial*> pMaterials;

protected:

	GGiGameTimer* pTimer;

	int mClientWidth = 800;
	int mClientHeight = 600;

	std::unique_ptr<GRiRendererFactory> mFactory;

	std::unique_ptr<GRiFilmboxManager> mFilmboxManager;

	// Set true to use 4X MSAA.  The default is false.
	//bool      m4xMsaaState = false;    // 4X MSAA enabled
	//UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

};

