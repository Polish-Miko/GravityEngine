
#pragma once

#define USE_IMGUI

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "GRiInclude.h"
#include "GDxPreInclude.h"
#include "GDxUploadBuffer.h"
#include "GDxFrameResource.h"
#include "GDxCubeRtv.h"
#include "GDxRtvHeap.h"
#include "GDxImgui.h"
#include "GDxGpuProfiler.h"
#include "GDxUav.h"
#include "../Shaders/ShaderDefinition.h"
#include "GDxReadbackBuffer.h"
#include "GDxCascadedShadowMap.h"

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#define TAA_SAMPLE_COUNT 8;
#define TAA_JITTER_DISTANCE 1.0;

#define SKY_CUBEMAP_SIZE 1024

#define USE_MASKED_DEPTH_BUFFER 1

// should be the same with TiledDeferredCS.hlsl
//#define DEFER_TILE_SIZE_X 16
//#define DEFER_TILE_SIZE_Y 16

//#define DEFER_CLUSTER_SIZE_X 32
//#define DEFER_CLUSTER_SIZE_Y 32
//#define DEFER_CLUSTER_NUM_Z 16

#define USE_CBDR 1

#if USE_CBDR
#define USE_TBDR 0
#else
#define USE_TBDR 1
#endif

struct LightList
{
	unsigned int PointLightIndices[MAX_GRID_POINT_LIGHT_NUM];
	unsigned int NumPointLights;
	unsigned int SpotLightIndices[MAX_GRID_SPOTLIGHT_NUM];
	unsigned int NumSpotlights;
};

struct SdfList
{
	unsigned int NumSdf;
	unsigned int SdfObjIndices[MAX_GRID_SDF_NUM];
};

struct MeshSdfDescriptor
{
	DirectX::XMFLOAT3 Center;
	float HalfExtent;
	float Radius;
	int Resolution;
};

// 8x TAA
static const double Halton_2[8] =
{
	0.0,
	-1.0 / 2.0,
	1.0 / 2.0,
	-3.0 / 4.0,
	1.0 / 4.0,
	-1.0 / 4.0,
	3.0 / 4.0,
	-7.0 / 8.0
};

// 8x TAA
static const double Halton_3[8] =
{
	-1.0 / 3.0,
	1.0 / 3.0,
	-7.0 / 9.0,
	-1.0 / 9.0,
	5.0 / 9.0,
	-5.0 / 9.0,
	1.0 / 9.0,
	7.0 / 9.0
};

class GDxRenderer : public GRiRenderer
{

protected:

	GDxRenderer(const GDxRenderer& rhs) = delete;
	GDxRenderer& operator=(const GDxRenderer& rhs) = delete;
	virtual ~GDxRenderer();

public:

	static GDxRenderer& GetRenderer();

	virtual void PreInitialize(HWND OutputWindow, double width, double height) override;

	virtual void Initialize() override;

	virtual bool IsRunning() override;
	
	virtual void OnResize() override;

	virtual void RegisterTexture(GRiTexture* text) override;

	virtual void CreateRendererFactory() override;
	virtual void CreateFilmboxManager() override;

	virtual void SetImgui(GRiImgui* imguiPtr) override;

	virtual GRiSceneObject* SelectSceneObject(int sx, int sy) override;

	virtual std::vector<ProfileData> GetGpuProfiles() override;

protected:

	virtual void CreateRtvAndDsvDescriptorHeaps();
	virtual void Update(const GGiGameTimer* gt) override;
	virtual void Draw(const GGiGameTimer* gt) override;

	void ScriptUpdate(const GGiGameTimer* gt);

	void UpdateObjectCBs(const GGiGameTimer* gt);
	void UpdateMaterialBuffer(const GGiGameTimer* gt);
	void UpdateSdfDescriptorBuffer(const GGiGameTimer* gt);
	void UpdateShadowTransform(const GGiGameTimer* gt);
	void UpdateMainPassCB(const GGiGameTimer* gt);
	void UpdateSkyPassCB(const GGiGameTimer* gt);
	void UpdateLightCB(const GGiGameTimer* gt);
	void CullSceneObjects(const GGiGameTimer* gt);

	void InitializeGpuProfiler();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildPSOs();
	void BuildFrameResources();

	void CubemapPreIntegration();

	void BuildMeshSDF();

	//void SaveBakedCubemap(std::wstring workDir, std::wstring CubemapPath);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuSrv(int index)const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuSrv(int index)const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(int index)const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(int index)const;

	void DrawSceneObjects(ID3D12GraphicsCommandList* cmdList, const RenderLayer layer, bool bSetObjCb, bool bSetSubmeshCb, bool bCheckCullState = false);
	void DrawSceneObject(ID3D12GraphicsCommandList* cmdList, GRiSceneObject* sObject, bool bSetObjCb, bool bSetSubmeshCb, bool bCheckCullState = false);

	float GetHaltonValue(int index, int radix);
	DirectX::XMFLOAT2 GenerateRandomHaltonOffset();

protected:

	bool InitDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();

	void FlushCommandQueue();

	void ResetCommandList();
	void ExecuteCommandList();

	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:

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
	//Microsoft::WRL::ComPtr<ID3D12Resource> mStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthReadbackBuffer;

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

protected:

	std::vector<std::unique_ptr<GDxFrameResource>> mFrameResources;
	GDxFrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	ComPtr<ID3D12RootSignature> mSsaoRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	//ComPtr<ID3D12DescriptorHeap> mSdfSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<GDxUav>> mUavs;
	std::unordered_map<std::string, std::unique_ptr<GDxRtvHeap>> mRtvHeaps;
	std::unordered_map<std::string, std::unique_ptr<GDxCubeRtv>> mCubeRtvs;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> mRootSignatures;

	GDxImgui* pImgui = nullptr;

	UINT mTextrueHeapIndex = 0;
	UINT mSdfTextrueIndex = 0;
	UINT mSkyTexHeapIndex = 0;
	UINT mShadowMapHeapIndex = 0;
	UINT mSsaoHeapIndexStart = 0;
	UINT mSsaoAmbientMapIndex = 0;

	UINT mNullCubeSrvIndex = 0;
	UINT mNullTexSrvIndex1 = 0;
	UINT mNullTexSrvIndex2 = 0;

	//UINT mDepthSrvIndex = 0;
	UINT mDepthBufferSrvIndex = 0;
	UINT mDepthDownsampleSrvIndex = 0;
	UINT mStencilBufferSrvIndex = 0;
	UINT mVelocityBufferSrvIndex = 0;
	UINT mGBufferSrvIndex = 0;
	UINT mTileClusterSrvIndex = 0;
	UINT mSdfTileSrvIndex = 0;
	UINT mCascadedShadowMapSrvIndex = 0;
	UINT mXPrefilteredShadowMapUavIndex = 0;
	UINT mYPrefilteredShadowMapUavIndex = 0;
	UINT mXPrefilteredShadowMapSrvIndex = 0;
	UINT mYPrefilteredShadowMapSrvIndex = 0;
	UINT mScreenSpaceShadowPassSrvIndex = 0;
	UINT mSSShadowTemporalSrvIndex = 0;
	UINT mGtaoSrvIndex = 0;
	UINT mLightPassSrvIndex = 0;
	UINT mSkyPassSrvIndex = 0;
	UINT mTaaPassSrvIndex = 0;
	UINT mSsrSrvIndex = 0;
	UINT mMotionBlurSrvIndex = 0;
	UINT mDofSrvIndex = 0;
	UINT mBloomSrvIndex = 0;
	UINT mIblIndex = 0;
	UINT mBlueNoiseSrvIndex = 0;

	UINT mDepthDsvIndex = 0;
	UINT mShadowMapDsvIndex = 0;

	UINT mGBufferAlbedoSrvIndexOffert = 0;
	UINT mGBufferNormalSrvIndexOffert = 0;
	UINT mGBufferOrmSrvIndexOffert = 0;
	UINT mGBufferVelocitySrvIndexOffert = 0;

	UINT mMotionBlurVdBufferSrvIndexOffset = 0;
	UINT mMotionBlurOutputSrvIndexOffset = 0;
	UINT mMotionBlurFirstTileMaxSrvIndexOffset = 0;
	UINT mMotionBlurSecondTileMaxSrvIndexOffset = 0;
	UINT mMotionBlurThirdTileMaxSrvIndexOffset = 0;
	UINT mMotionBlurFourthTileMaxSrvIndexOffset = 0;
	UINT mMotionBlurNeighborMaxSrvIndexOffset = 0;

	UINT mBloomDownSrvIndexOffset = 0;
	UINT mBloomUpSrvIndexOffset = 0;
	UINT mBloomOutputSrvIndexOffset = 0;
	const int BloomChainLength = 12;
	int BloomIterations = 0;
	int BloomWidth = 0;
	int BloomHeight = 0;
	float BloomSampleScale = 0.0f;

	UINT mGtaoRawSrvIndexOffset = 0;
	UINT mGtaoUpsampleSrvIndexOffset = 0;
	UINT mGtaoBlurXSrvIndexOffset = 0;
	UINT mGtaoBlurYSrvIndexOffset = 0;
	UINT mGtaoHistory1SrvIndexOffset = 0;
	UINT mGtaoHistory2SrvIndexOffset = 0;
	UINT mGtaoTemporalSrvIndexOffset = 0;

	UINT mSsrUnjitteredDepthSrvIndexOffset = 0;
	UINT mSsrHizSrvIndexOffset = 0;
	UINT mSsrPrefilterSrvIndexOffset = 0;
	UINT mSsrTileImportanceSrvIndexOffset = 0;
	UINT mSsrTileImportanceHistorySrvIndexOffset = 0;
	UINT mSsrTileImportanceTemporalSrvIndexOffset = 0;
	UINT mSsrTraceSrvIndexOffset = 0;
	UINT mSsrTraceMaskSrvIndexOffset = 0;
	UINT mSsrResolveSrvIndexOffset = 0;
	UINT mSsrHistorySrvIndexOffset = 0;
	UINT mSsrTemporalSrvIndexOffset = 0;
	UINT mSsrCombineSrvIndexOffset = 0;

	UINT mDofCocSrvIndexOffset = 0;
	UINT mDofPrefilterSrvIndexOffset = 0;
	UINT mDofBokehSrvIndexOffset = 0;
	UINT mDofPostfilterSrvIndexOffset = 0;
	UINT mDofCombineSrvIndexOffset = 0;

	UINT mLightingSrvIndexOffset = 0;
	UINT mLightingAmbientSpecularSrvIndexOffset = 0;

	float mGtaoTemporalRotations[6] = { 60, 300, 180, 240, 120, 0 };
	float mGtaoSpatialOffsets[4] = { 0, 0.5f, 0.25f, 0.75f };
	int mGtaoSampleStep = 0;

	int numVisible = 0;
	int numFrustumCulled = 0;
	int numOcclusionCulled = 0;

	int mHaltonSampleIndex = 0;
	static const int mHaltonSampleCount = 64;

	const float mDofFilmHeight = 0.024f;
	float mDofDistance = 0.0f;
	float mDofCoeff = 0.0f;
	float mDofMaxCoc = 0.0f;
	float mDofRcpAspect = 0.0f;

	int ShadowCascadeNum = SHADOW_CASCADE_NUM;
	std::vector<float> ShadowCascadeDistance = { Z_LOWER_BOUND, Z_LOWER_BOUND + 1000.0F, Z_LOWER_BOUND + 3000.0f };
	float ShadowMapCameraDis = (LIGHT_Z_UPPER_BOUND - LIGHT_Z_LOWER_BOUND) / 2.0f;
	UINT64 ShadowMapResolution = SHADOW_MAP_RESOLUTION;
	XMMATRIX mShadowView[SHADOW_CASCADE_NUM];
	XMMATRIX mShadowProj[SHADOW_CASCADE_NUM];
	XMMATRIX mShadowViewProj[SHADOW_CASCADE_NUM];
	XMMATRIX mShadowTransform[SHADOW_CASCADE_NUM];

	D3D12_VIEWPORT ShadowViewport;
	D3D12_RECT ShadowScissorRect;

	XMMATRIX mSdfTileSpaceTransform;
	XMMATRIX mSdfTileSpaceView;
	float mSdfTileSpaceWidth, mSdfTileSpaceHeight;

	GGiFloat3 MainDirectionalLightDir = GGiFloat3(0.57735f, -0.57735f, -0.57735f);

	UINT mTaaHistoryIndex = 0;
	UINT mShadowTemporalHistoryIndex = 0;
	UINT mGtaoTemporalHistoryIndex = 0;
	UINT mSsrTemporalHistoryIndex = 0;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;

	PassConstants mMainPassCB;  // index 0 of pass cbuffer.
	PassConstants mShadowPassCB;// index 1 of pass cbuffer.
	SkyPassConstants mSkyPassCB;

	std::vector<std::unique_ptr<GDxUploadBuffer<SkyPassConstants>>> PreIntegrationPassCbs;

	MeshSdfDescriptor mMeshSdfDescriptors[MAX_MESH_NUM];
	SceneObjectSdfDescriptor mSceneObjectSdfDescriptors[MAX_SCENE_OBJECT_NUM];

	std::unique_ptr<GDxUploadBuffer<MeshSdfDescriptor>> mMeshSdfDescriptorBuffer;

	Microsoft::WRL::ComPtr<ID3D12Resource> mSdfTextures[MAX_SCENE_OBJECT_NUM] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mSdfTextureUploadBuffer[MAX_SCENE_OBJECT_NUM] = { nullptr };

	UINT mSceneObjectSdfNum = 0;

	DirectX::BoundingSphere mSceneBounds;

	float mLightNearZ = 0.0f;
	float mLightFarZ = 0.0f;
	XMFLOAT3 mLightPosW;
	XMFLOAT4X4 mLightView = GDxMathHelper::Identity4x4();
	XMFLOAT4X4 mLightProj = GDxMathHelper::Identity4x4();
	XMFLOAT4X4 mLightTransform = GDxMathHelper::Identity4x4();

	float mLightRotationAngle = 0.0f;
	XMFLOAT3 mBaseLightDirections[3] = {
		XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(0.0f, -0.707f, -0.707f)
	};
	XMFLOAT3 mRotatedLightDirections[3];

	POINT mLastMousePos;

	std::unique_ptr<GGiThreadPool> mRendererThreadPool = nullptr;

	//std::shared_ptr<GRiKdTree> mAcceleratorTree = nullptr;

	std::vector<std::unique_ptr<GDxCascadedShadowMap>> mCascadedShadowMap;

private:
	
	GDxRenderer();

};

