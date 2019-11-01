#pragma once

#include "GDxPreInclude.h"
#include "GDxMathHelper.h"
#include "GDxUploadBuffer.h"

// should be the same with the definition in Lighting.hlsli
#define MAX_DIRECTIONAL_LIGHT_NUM 4
#define MAX_POINT_LIGHT_NUM 1024
#define MAX_SPOTLIGHT_NUM 1024

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = GDxMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 PrevWorld = GDxMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvTransWorld = GDxMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = GDxMathHelper::Identity4x4();
	//UINT     MaterialIndex;
	//UINT     ObjPad0;
	//UINT     ObjPad1;
	//UINT     ObjPad2;
};

struct PassConstants
{
	DirectX::XMFLOAT4X4 View = GDxMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = GDxMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = GDxMathHelper::Identity4x4();//Jittered
	DirectX::XMFLOAT4X4 InvProj = GDxMathHelper::Identity4x4();//Jittered
	DirectX::XMFLOAT4X4 ViewProj = GDxMathHelper::Identity4x4();//Jittered
	DirectX::XMFLOAT4X4 UnjitteredViewProj = GDxMathHelper::Identity4x4();//Unjittered
	DirectX::XMFLOAT4X4 InvViewProj = GDxMathHelper::Identity4x4();//Jittered
	DirectX::XMFLOAT4X4 PrevViewProj = GDxMathHelper::Identity4x4();//Unjittered
	DirectX::XMFLOAT4X4 ViewProjTex = GDxMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ShadowTransform = GDxMathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;
	UINT FrameCount = 0u;
	DirectX::XMFLOAT2 Jitter = { 0.0f,0.0f };
	UINT     Pad0;
	
	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT4 MainDirectionalLightDir = { 0.0f, -1.0f, 0.0f, 0.0f };
};

struct LightConstants
{
	GRiDirectionalLight dirLight[MAX_DIRECTIONAL_LIGHT_NUM];
	GRiPointLight pointLight[MAX_POINT_LIGHT_NUM];
	DirectX::XMFLOAT3 cameraPosition;
	int pointLightCount;
	int dirLightCount;
};

struct SkyPassConstants
{
	DirectX::XMFLOAT4X4 ViewProj = GDxMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 PrevViewProj = GDxMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 UnjitteredViewProj = GDxMathHelper::Identity4x4();
	DirectX::XMFLOAT3 PrevPos = { 0.0f, 0.0f, 0.0f };
	float pad1 = 0.0f;
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float roughness = 0.0f;
};

struct SsaoConstants
{
	DirectX::XMFLOAT4X4 Proj;
	DirectX::XMFLOAT4X4 InvProj;
	DirectX::XMFLOAT4X4 ProjTex;
	DirectX::XMFLOAT4   OffsetVectors[14];

	// For SsaoBlur.hlsl
	DirectX::XMFLOAT4 BlurWeights[3];

	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };

	// Coordinates given in view space.
	float OcclusionRadius = 0.5f;
	float OcclusionFadeStart = 0.2f;
	float OcclusionFadeEnd = 2.0f;
	float SurfaceEpsilon = 0.05f;
};

struct MaterialData
{
	// Used in texture mapping.
	DirectX::XMFLOAT4X4 MatTransform = GDxMathHelper::Identity4x4();

	UINT TextureIndex[MATERIAL_MAX_TEXTURE_NUM];
	//UINT TextureSrgb[MATERIAL_MAX_TEXTURE_NUM];
	float ScalarParams[MATERIAL_MAX_SCALAR_NUM];
	DirectX::XMFLOAT4 VectorParams[MATERIAL_MAX_VECTOR_NUM];
};

struct SceneObjectSdfDescriptor
{
	DirectX::XMFLOAT4X4 objWorld;
	DirectX::XMFLOAT4X4 objInvWorld;
	DirectX::XMFLOAT4X4 objInvWorld_IT;
	int SdfIndex;
};

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 TexC;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 TangentU;
};

// Stores the resources needed for the CPU to build the command lists
// for a frame.  
struct GDxFrameResource
{
public:

	GDxFrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount);
	GDxFrameResource(const GDxFrameResource& rhs) = delete;
	GDxFrameResource& operator=(const GDxFrameResource& rhs) = delete;
	~GDxFrameResource();

	// We cannot reset the allocator until the GPU is done processing the commands.
	// So each frame needs their own allocator.
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	// We cannot update a cbuffer until the GPU is done processing the commands
	// that reference it.  So each frame needs their own cbuffers.
	std::unique_ptr<GDxUploadBuffer<PassConstants>> PassCB = nullptr;
	std::unique_ptr<GDxUploadBuffer<ObjectConstants>> ObjectCB = nullptr;
	std::unique_ptr<GDxUploadBuffer<SsaoConstants>> SsaoCB = nullptr;
	std::unique_ptr<GDxUploadBuffer<LightConstants>> LightCB = nullptr;
	std::unique_ptr<GDxUploadBuffer<SkyPassConstants>> SkyCB = nullptr;

	std::unique_ptr<GDxUploadBuffer<MaterialData>> MaterialBuffer = nullptr;
	std::unique_ptr<GDxUploadBuffer<SceneObjectSdfDescriptor>> SceneObjectSdfDescriptorBuffer = nullptr;

	// Fence value to mark commands up to this fence point.  This lets us
	// check if these frame resources are still in use by the GPU.
	UINT64 Fence = 0;
};

