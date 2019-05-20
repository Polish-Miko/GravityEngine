#pragma once

#include "GDxRenderer.h"

#include "GProject.h"
#include "GScene.h"
#include "GMaterial.h"
#include "GGuiCallback.h"
#include "Shaders/FrustumZ.h"




using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

// Win32 message handler
#ifdef USE_IMGUI
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

class GCore
{
public:
	GCore(const GCore& rhs) = delete;
	GCore& operator=(const GCore& rhs) = delete;
	~GCore();

	static GCore& GetCore();

	void Run();

	void Initialize(HWND OutputWindow, double width, double height);

	void MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#pragma region export

	void SaveProject();

	int GetSceneObjectNum();

	const wchar_t* GetSceneObjectName(int index);

	void GetSceneObjectTransform(wchar_t* objName, float* trans);

	void SetSceneObjectTransform(wchar_t* objName, float* trans);

	void SetWorkDirectory(wchar_t* dir);

	void SetProjectName(wchar_t* projName);

	bool GetTextureSrgb(wchar_t* txtName);

	void SetTextureSrgb(wchar_t* txtName, bool bSrgb);

	void CreateMaterial(wchar_t* cUniqueName);

	void GetMaterialScale(wchar_t* matUniqueName, float* scale);

	void SetMaterialScale(wchar_t* matUniqueName, float* scale);

	const wchar_t* GetMaterialTextureUniqueName(wchar_t* matUniqueName, int index);

	bool SetMaterialTexture(wchar_t* matUniqueName, int index, wchar_t* texUniqueName);

	void SetMaterialTextureToDefaultValue(wchar_t* matUniqueName, int index);

	void RenameMaterial(wchar_t* oldUniqueName, wchar_t* newUniqueName);

	void SetSceneObjectMesh(wchar_t* sceneObjectName, wchar_t* meshUniqueName);

	void SetSceneObjectMaterial(wchar_t* sceneObjectName, wchar_t* matUniqueName);

	const wchar_t* GetSceneObjectMeshName(wchar_t* sceneObjectName);

	const wchar_t* GetSceneObjectMaterialName(wchar_t* sceneObjectName);

	bool SceneObjectExists(wchar_t* sceneObjectName);

	void CreateSceneObject(wchar_t* sceneObjectName, wchar_t* meshUniqueName);

	void RenameSceneObject(wchar_t* oldName, wchar_t* newName);

	void DeleteSceneObject(wchar_t* sceneObjectName);

	const wchar_t* GetSkyCubemapUniqueName();

	const wchar_t* GetDefaultSkyCubemapUniqueName();

	void SetSkyCubemapUniqueName(wchar_t* newName);

	bool SkyCubemapNameAvailable(wchar_t* cubemapName);

	void SetSelectSceneObjectCallback(VoidWstringFuncPointerType callback);

	void SelectSceneObject(wchar_t* sceneObjectName);

	void SetRefreshSceneObjectTransformCallback(VoidFuncPointerType pRefreshSceneObjectTransformCallback);

#pragma endregion

private:

	bool      mAppPaused = false;  // is the application paused?
	bool      mMinimized = false;  // is the application minimized?
	bool      mMaximized = false;  // is the application maximized?
	bool      mResizing = false;   // are the resize bars being dragged?
	bool      mFullscreenState = false;// fullscreen enabled

	std::wstring WorkDirectory;

	std::wstring EngineDirectory;

	std::wstring ProjectName;

	GRiRenderer* mRenderer;

	GRiRendererFactory* pRendererFactory;

	std::unique_ptr<GGiGameTimer> mTimer;

	std::unordered_map<std::wstring, std::unique_ptr<GRiTexture>> mTextures;
	std::unordered_map<std::wstring, std::unique_ptr<GRiMaterial>> mMaterials;
	std::unordered_map<std::wstring, std::unique_ptr<GRiMesh>> mMeshes;
	std::unordered_map<std::wstring, std::unique_ptr<GRiSceneObject>> mSceneObjects;
	std::vector<GRiSceneObject*> mSceneObjectLayer[(int)RenderLayer::Count];
	std::unordered_map<std::wstring, std::unique_ptr<GMaterial>> mMaterialFiles;

	std::unique_ptr<GRiCamera> mCamera;

	std::unique_ptr<GRiCamera> mCubemapSampleCamera[6];

	POINT mLastMousePos;

	GProject* mProject;

	std::unique_ptr<GRiImgui> mImgui;

	UINT mMaterialIndex = 0;

	UINT mSceneObjectIndex = 0;

	std::wstring mSkyCubemapUniqueName;

	GRiSceneObject* mSelectedSceneObject = nullptr;

	GGuiCallback* mGuiCallback;

	float mCameraSpeed = 150.0f;

private:

	GCore();
	
	void OnResize();

	void Update();

	void RecordPrevFrame(const GGiGameTimer* gt);
	void OnKeyboardInput(const GGiGameTimer* gt);
	void UpdateGui(const GGiGameTimer* gt);
	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

	void CreateImgui();
	void LoadTextures();
	void LoadMaterials();
	void LoadMeshes();
	void LoadSceneObjects();
	void LoadCameras();

	void LoadProject();

	void Pick(int sx, int sy);

	//Util
	std::vector<std::wstring> GetAllFilesInFolder(std::wstring path, bool bCheckFormat, std::vector<std::wstring> format);
	std::vector<std::wstring> GetAllFilesUnderFolder(std::wstring path, bool bCheckFormat, std::vector<std::wstring> format);

};
