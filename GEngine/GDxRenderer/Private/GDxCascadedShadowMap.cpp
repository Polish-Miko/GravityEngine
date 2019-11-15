#include "stdafx.h"
#include "GDxCascadedShadowMap.h"


GDxCascadedShadowMap::GDxCascadedShadowMap(ID3D12Device* pDevice, UINT res)
{
	Resolution = res;

	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = Resolution;
	depthStencilDesc.Height = Resolution;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = DXGI_FORMAT_R32G8X24_TYPELESS; //DXGI_FORMAT_R24G8_TYPELESS;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT; //DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mShadowmapResource.GetAddressOf())));

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	texDesc.Width = (UINT)(Resolution);
	texDesc.Height = (UINT)(Resolution);
	texDesc.Format = DXGI_FORMAT_R32_FLOAT;

	// Free the old resources if they exist.
	if (mXPrefilteredShadowmapResource != nullptr)
		mXPrefilteredShadowmapResource.Reset();
	if (mYPrefilteredShadowmapResource != nullptr)
		mYPrefilteredShadowmapResource.Reset();

	float clearColor[4];
	clearColor[0] = 0.0f;
	clearColor[1] = 0.0f;
	clearColor[2] = 0.0f;
	clearColor[3] = 1.0f;
	CD3DX12_CLEAR_VALUE optPrefilteredClear(texDesc.Format, clearColor);

	ThrowIfFailed(pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optPrefilteredClear,
		IID_PPV_ARGS(&mXPrefilteredShadowmapResource)));

	ThrowIfFailed(pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optPrefilteredClear,
		IID_PPV_ARGS(&mYPrefilteredShadowmapResource)));
}


GDxCascadedShadowMap::~GDxCascadedShadowMap()
{
}

ID3D12Resource* GDxCascadedShadowMap::GetShadowmapResource()
{
	return mShadowmapResource.Get();
}

ID3D12Resource* GDxCascadedShadowMap::GetXPrefilteredResource()
{
	return mXPrefilteredShadowmapResource.Get();
}

ID3D12Resource* GDxCascadedShadowMap::GetYPrefilteredResource()
{
	return mYPrefilteredShadowmapResource.Get();
}


