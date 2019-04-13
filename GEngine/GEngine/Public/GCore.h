
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

//typedef void(__stdcall * VoidFuncPointerType)(void);

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

	//int GetSceneObjectNum();

	//const char* GetSceneObjectName(int index);

	//void SetSetSceneObjectsCallback(VoidFuncPointerType pSetSceneObjectsCallback);

	//void GetSceneObjectTransform(char* objName, float* trans);

	//void SetSceneObjectTransform(char* objName, float* trans);

#pragma endregion

private:

	//HINSTANCE mhAppInst = nullptr; // application instance handle
	bool      mAppPaused = false;  // is the application paused?
	bool      mMinimized = false;  // is the application minimized?
	bool      mMaximized = false;  // is the application maximized?
	bool      mResizing = false;   // are the resize bars being dragged?
	bool      mFullscreenState = false;// fullscreen enabled

	void Update();
	//void Draw();

	//void SetWorkDirectory();
	//void LoadTextures();

	//Util
	//std::vector<std::wstring> GetAllFilesInFolder(std::wstring path, bool bCheckFormat, std::vector<std::wstring> format);
	//std::vector<std::wstring> GetAllFilesUnderFolder(std::wstring path, bool bCheckFormat, std::vector<std::wstring> format);

private:

	GCore();

	GRiRenderer* mRenderer;

	std::unique_ptr<GGiGameTimer> mTimer;

	//std::wstring WorkDirectory;

	/*
#pragma region Export-Related

	VoidFuncPointerType mSetSceneObjectsCallback;

	void SetSceneObjectsCallback();

#pragma endregion
	*/

};