//***************************************************************************************
// d3dApp.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#pragma once


#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "GRiInclude.h"
#include "GDxUploadBuffer.h"
#include "GDxFrameResource.h"
#include "GDxCubeRtv.h"
#include "GDxRtvHeap.h"

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

class GDxRenderer : public GRiRenderer
{
protected:

	GDxRenderer(const GDxRenderer& rhs) = delete;
	GDxRenderer& operator=(const GDxRenderer& rhs) = delete;
	virtual ~GDxRenderer();

public:

	//GRenderer();

	static GDxRenderer& GetRenderer();

	//static GRenderer* GetApp();

	//HINSTANCE AppInst()const;
	HWND      MainWnd()const;
	//float     AspectRatio()const;

	//bool Get4xMsaaState()const;
	//void Set4xMsaaState(bool value);

	//int Run();

	virtual void PreInitialize(HWND OutputWindow, double width, double height) override;

	virtual void Initialize(HWND OutputWindow, double width, double height) override;

	virtual bool IsRunning() override;
	
	virtual void OnResize() override;

	// Convenience overrides for handling mouse input.
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	virtual void CreateRendererFactory() override;
	virtual void CreateFilmboxManager() override;

	virtual void SyncTextures(std::unordered_map<std::wstring, std::unique_ptr<GRiTexture>>& mTextures) override;
	virtual void SyncMaterials(std::unordered_map<std::wstring, std::unique_ptr<GRiMaterial>>& mMaterials) override;
	virtual void SyncMeshes(std::unordered_map<std::wstring, std::unique_ptr<GRiMesh>>& mMeshes) override;
	virtual void SyncSceneObjects(std::unordered_map<std::wstring, std::unique_ptr<GRiSceneObject>>& mSceneObjects, std::vector<GRiSceneObject*>* mSceneObjectLayer) override;
	virtual void SyncCameras(std::vector<GRiCamera*> mCameras) override;

	//virtual bool Initialize(HWND OutputWindow, double width, double height);

	//virtual void MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	virtual void CreateRtvAndDsvDescriptorHeaps();
	//virtual void Update(const GameTimer& gt) = 0;
	//virtual void Draw(const GameTimer& gt) = 0;
	//virtual void Draw_Test(const GameTimer& gt) = 0;
	virtual void Update(const GGiGameTimer* gt) override;
	virtual void Draw(const GGiGameTimer* gt) override;

	void OnKeyboardInput(const GGiGameTimer* gt);
	void AnimateMaterials(const GGiGameTimer* gt);
	void UpdateObjectCBs(const GGiGameTimer* gt);
	void UpdateMaterialBuffer(const GGiGameTimer* gt);
	void UpdateShadowTransform(const GGiGameTimer* gt);
	void UpdateMainPassCB(const GGiGameTimer* gt);
	void UpdateSkyPassCB(const GGiGameTimer* gt);
	//void UpdateShadowPassCB(const GameTimer* gt);
	//void UpdateSsaoCB(const GameTimer* gt);
	void UpdateLightCB(const GGiGameTimer* gt);

	//void BuildCubemapSampleCameras();
	//void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout(); 
	//void LoadMeshes();
	void BuildPSOs();
	void BuildFrameResources();
	//void BuildMaterials();
	//void BuildSceneObjects();

	void CubemapPreIntegration();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuSrv(int index)const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuSrv(int index)const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(int index)const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(int index)const;

	void DrawSceneObjects(ID3D12GraphicsCommandList* cmdList, const RenderLayer layer, bool bSetCBV);
	void DrawSceneObject(ID3D12GraphicsCommandList* cmdList, GRiSceneObject* sObject, bool bSetCBV);

protected:

	//bool InitMainWindow();
	bool InitDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();

	void FlushCommandQueue();

	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	//void CalculateFrameStats();

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:

	HWND      mhMainWnd = nullptr; // main window handle

	/*
	//HINSTANCE mhAppInst = nullptr; // application instance handle
	HWND      mhMainWnd = nullptr; // main window handle
	bool      mAppPaused = false;  // is the application paused?
	bool      mMinimized = false;  // is the application minimized?
	bool      mMaximized = false;  // is the application maximized?
	bool      mResizing = false;   // are the resize bars being dragged?
	bool      mFullscreenState = false;// fullscreen enabled

	// Set true to use 4X MSAA (?.1.8).  The default is false.
	bool      m4xMsaaState = false;    // 4X MSAA enabled
	UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA
	*/

	// Used to keep track of the Delta-time and game time (?.4).
	//GGiGameTimer* pTimer;

	Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	// Derived class should set these in derived constructor to customize starting values.
	std::wstring mMainWndCaption = L"d3d App";
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//int mClientWidth = 800;
	//int mClientHeight = 600;

protected:
	std::vector<std::unique_ptr<GDxFrameResource>> mFrameResources;
	GDxFrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	//ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12RootSignature> mSsaoRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::wstring, GRiMesh*> pMeshes;
	//std::unordered_map<std::string, std::shared_ptr<GMaterial>> mMaterials;
	//std::unordered_map<std::string, std::unique_ptr<Material>> mLegMaterials;
	//std::unordered_map<std::string, std::shared_ptr<GTexture>> mTextures;
	//std::vector<std::shared_ptr<GTexture>> mTextureList;
	//std::unordered_map<std::string, std::unique_ptr<Texture>> mLegTextures;
	std::unordered_map<std::string, std::unique_ptr<GDxRtvHeap>> mRtvHeaps;
	std::unordered_map<std::string, std::unique_ptr<GDxCubeRtv>> mCubeRtvs;
	UINT mPrefilterLevels = 5u;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> mRootSignatures;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// List of all the render items.
	std::unordered_map<std::wstring, GRiSceneObject*> pSceneObjects;

	// Render items divided by PSO.
	std::vector<GRiSceneObject*> pSceneObjectLayer[(int)RenderLayer::Count];

	UINT mTextrueHeapIndex = 0;
	UINT mSkyTexHeapIndex = 0;
	UINT mShadowMapHeapIndex = 0;
	UINT mSsaoHeapIndexStart = 0;
	UINT mSsaoAmbientMapIndex = 0;

	UINT mNullCubeSrvIndex = 0;
	UINT mNullTexSrvIndex1 = 0;
	UINT mNullTexSrvIndex2 = 0;

	UINT mGBufferSrvIndex = 0;
	UINT mLightPassSrvIndex = 0;
	UINT mIblIndex = 0;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;

	PassConstants mMainPassCB;  // index 0 of pass cbuffer.
	PassConstants mShadowPassCB;// index 1 of pass cbuffer.
	SkyPassConstants mSkyPassCB;

	std::vector<std::unique_ptr<GDxUploadBuffer<SkyPassConstants>>> PreIntegrationPassCbs;

	//std::unique_ptr<ShadowMap> mShadowMap;

	//std::unique_ptr<Ssao> mSsao;

	DirectX::BoundingSphere mSceneBounds;

	float mLightNearZ = 0.0f;
	float mLightFarZ = 0.0f;
	XMFLOAT3 mLightPosW;
	XMFLOAT4X4 mLightView = GDxMathHelper::Identity4x4();
	XMFLOAT4X4 mLightProj = GDxMathHelper::Identity4x4();
	XMFLOAT4X4 mShadowTransform = GDxMathHelper::Identity4x4();

	float mLightRotationAngle = 0.0f;
	XMFLOAT3 mBaseLightDirections[3] = {
		XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(0.0f, -0.707f, -0.707f)
	};
	XMFLOAT3 mRotatedLightDirections[3];

	POINT mLastMousePos;

private:
	
	GDxRenderer();

	//void LoadTextures();

};

