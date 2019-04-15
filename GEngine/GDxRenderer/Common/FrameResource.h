#pragma once

//#include "GUtilInclude.h"
#include "GDxPreInclude.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "GLight.h"
#include "GMaterial.h"
#include "GDirectionalLight.h"
#include "GPointLight.h"
#include "GSpotlight.h"

// should be the same with Lighting.hlsli
#define MAX_DIRECTIONAL_LIGHT_NUM 4
#define MAX_POINT_LIGHT_NUM 16
#define MAX_SPOTLIGHT_NUM 16

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	UINT     MaterialIndex;
	UINT     ObjPad0;
	UINT     ObjPad1;
	UINT     ObjPad2;
};

struct PassConstants
{
	DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProjTex = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ShadowTransform = MathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of MaxLights per object.
	//GLight Lights[MaxLights];
};

struct LightConstants
{
	GDirectionalLight dirLight[MAX_DIRECTIONAL_LIGHT_NUM];
	GPointLight pointLight[MAX_POINT_LIGHT_NUM];
	//GSpotlight spotLight[MAX_SPOTLIGHT_NUM];
	//DirectX::XMFLOAT4X4 invProjView;
	DirectX::XMFLOAT3 cameraPosition;
	int pointLightCount;
	//int pointLightIndex;
	int dirLightCount;
	//int dirLightIndex;
};

struct SkyPassConstants
{
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
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
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();

	UINT TextureIndex[MATERIAL_MAX_TEXTURE_NUM];
	//UINT TextureSrgb[MATERIAL_MAX_TEXTURE_NUM];
	float ScalarParams[MATERIAL_MAX_SCALAR_NUM];
	DirectX::XMFLOAT4 VectorParams[MATERIAL_MAX_VECTOR_NUM];

	//DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	//DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	//float Roughness = 0.5f;


	//UINT DiffuseMapIndex = 0;
	//UINT NormalMapIndex = 0;
	//UINT MaterialPad1;
	//UINT MaterialPad2;
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
struct FrameResource
{
public:

	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource();

	// We cannot reset the allocator until the GPU is done processing the commands.
	// So each frame needs their own allocator.
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	// We cannot update a cbuffer until the GPU is done processing the commands
	// that reference it.  So each frame needs their own cbuffers.
	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
	std::unique_ptr<UploadBuffer<SsaoConstants>> SsaoCB = nullptr;
	std::unique_ptr<UploadBuffer<LightConstants>> LightCB = nullptr;
	std::unique_ptr<UploadBuffer<SkyPassConstants>> SkyCB = nullptr;

	std::unique_ptr<UploadBuffer<MaterialData>> MaterialBuffer = nullptr;

	// Fence value to mark commands up to this fence point.  This lets us
	// check if these frame resources are still in use by the GPU.
	UINT64 Fence = 0;
};