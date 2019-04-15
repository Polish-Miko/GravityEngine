#pragma once

//#include "GUtilInclude.h"
#include "GDxPreInclude.h"
#include "GTexture.h"

//
// max number must be a multiple of 4, and must be the same with the macro defined in common.hlsl
//
#define MATERIAL_MAX_TEXTURE_NUM 16
#define MATERIAL_MAX_SCALAR_NUM 16
#define MATERIAL_MAX_VECTOR_NUM 16

/*
struct MaterialConstants
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;

	// Used in texture mapping.
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};
*/

class GMaterial
{
public:
	GMaterial(){}
	~GMaterial(){}

	// Unique material name for lookup.
	std::string Name;

	// Index into constant buffer corresponding to this material.
	int MatCBIndex = -1;

	// Index into SRV heap for diffuse texture.
	//int DiffuseSrvHeapIndex = -1;

	// Index into SRV heap for normal texture.
	//int NormalSrvHeapIndex = -1;

	//albedo/normal/OcclussionRoughnessMetallic
	//std::vector<std::shared_ptr<GTexture>> mTextures;
	std::vector<GRiTexture*> pTextures;

	// Dirty flag indicating the material has changed and we need to update the constant buffer.
	// Because we have a material constant buffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify a material we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = NUM_FRAME_RESOURCES;

	// Material constant buffer data used for shading.
	//DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	//DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	std::vector<float> ScalarParams;
	std::vector<DirectX::XMFLOAT4> VectorParams;
	//float Roughness = .25f;
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();

	/*
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;

	void BuildDescriptorHeap(ID3D12Device* device, UINT srvSize)
	{
		//
		// Create the SRV heap.
		//
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = mTextures.size();
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap)));

		//
		// Fill out the heap with actual descriptors.
		//
		CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(srvHeap->GetCPUDescriptorHandleForHeapStart());

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		for (UINT i = 0; i < (UINT)mTextures.size(); ++i)
		{
			srvDesc.Format = mTextures[i]->Resource->GetDesc().Format;
			srvDesc.Texture2D.MipLevels = mTextures[i]->Resource->GetDesc().MipLevels;
			device->CreateShaderResourceView(mTextures[i]->Resource.Get(), &srvDesc, hDescriptor);

			// next descriptor
			hDescriptor.Offset(1, srvSize);
		}
	}
	*/
};

