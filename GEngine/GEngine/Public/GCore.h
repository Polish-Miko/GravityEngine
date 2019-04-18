
#pragma once

#include "GDxRenderer.h"
//#include "MathHelper.h"
//#include "UploadBuffer.h"
//#include "GeometryGenerator.h"
//#include "Camera.h"
//#include "FrameResource.h"
//#include "ShadowMap.h"
//#include "Ssao.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

typedef void(__stdcall * VoidFuncPointerType)(void);

class GCore// : public GRenderer
{
public:
	GCore(const GCore& rhs) = delete;
	GCore& operator=(const GCore& rhs) = delete;
	~GCore();

	static GCore& GetCore();

	void Run();

	void Initialize(HWND OutputWindow, double width, double height);

	//virtual bool Initialize(HWND OutputWindow, double width, double height)override;

#pragma region export

	void MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	int GetSceneObjectNum();

	const wchar_t* GetSceneObjectName(int index);

	void SetSetSceneObjectsCallback(VoidFuncPointerType pSetSceneObjectsCallback);

	void GetSceneObjectTransform(wchar_t* objName, float* trans);

	void SetSceneObjectTransform(wchar_t* objName, float* trans);

#pragma endregion

private:

	//HINSTANCE mhAppInst = nullptr; // application instance handle
	bool      mAppPaused = false;  // is the application paused?
	bool      mMinimized = false;  // is the application minimized?
	bool      mMaximized = false;  // is the application maximized?
	bool      mResizing = false;   // are the resize bars being dragged?
	bool      mFullscreenState = false;// fullscreen enabled

	std::wstring WorkDirectory;

	GRiRenderer* mRenderer;

	GRiRendererFactory* pRendererFactory;

	std::unique_ptr<GGiGameTimer> mTimer;

	std::unordered_map<std::wstring, std::unique_ptr<GRiTexture>> mTextures;
	std::unordered_map<std::wstring, std::unique_ptr<GRiMaterial>> mMaterials;
	std::unordered_map<std::wstring, std::unique_ptr<GRiMesh>> mMeshes;
	std::unordered_map<std::wstring, std::unique_ptr<GRiSceneObject>> mSceneObjects;
	std::vector<GRiSceneObject*> mSceneObjectLayer[(int)RenderLayer::Count];

	//void Draw();

	//void SetWorkDirectory();

private:

	GCore();

	void Update();

	void LoadTextures();
	void LoadMaterials();
	void LoadSkyTexture(std::wstring path);
	void LoadMeshes();
	void LoadSceneObjects();

	void SetWorkDirectory();

	//Util
	std::vector<std::wstring> GetAllFilesInFolder(std::wstring path, bool bCheckFormat, std::vector<std::wstring> format);
	std::vector<std::wstring> GetAllFilesUnderFolder(std::wstring path, bool bCheckFormat, std::vector<std::wstring> format);

	/*
#pragma region Export-Related

	VoidFuncPointerType mSetSceneObjectsCallback;

	void SetSceneObjectsCallback();

#pragma endregion
	*/

#pragma region Export-Related

	VoidFuncPointerType mSetSceneObjectsCallback;

	void SetSceneObjectsCallback();

#pragma endregion

};