#pragma once

//#include "GUtilInclude.h"
#include "GDxPreInclude.h"

struct GRtvProperties
{
	DXGI_FORMAT mRtvFormat;

	// RtvWidth = viewportWidth * mWidthPercentage
	float mWidthPercentage = 1;
	float mHeightPercentage = 1;

	FLOAT mClearColor[4] = { 0,0,0,0 };
};

class GDxRtv
{

public:

	GDxRtv(GRtvProperties prop, UINT ClientWidth, UINT ClientHeight, bool ScaledByViewport = true)
	{
		mProperties = prop;

		mClientWidth = ClientWidth;
		mClientHeight = ClientHeight;

		bScaledByViewport = ScaledByViewport;

		if (bScaledByViewport)
		{
			SetViewportAndScissorRect((UINT)(max(ClientWidth * mProperties.mWidthPercentage, 1.0f)), (UINT)(max(ClientHeight * mProperties.mHeightPercentage, 1.0f)));
		}
		else
		{
			SetViewportAndScissorRect(ClientWidth, ClientHeight);
		}
	}

	~GDxRtv(){}

	void OnResize(UINT ClientWidth, UINT ClientHeight)
	{
		if (bScaledByViewport)
		{
			mClientWidth = ClientWidth;
			mClientHeight = ClientHeight;
			SetViewportAndScissorRect((UINT)(max(ClientWidth * mProperties.mWidthPercentage, 1.0f)), (UINT)(max(ClientHeight * mProperties.mHeightPercentage, 1.0f)));
		}
	}

	void SetViewportAndScissorRect(UINT Width, UINT Height)
	{
		mViewport.TopLeftX = 0.0f;
		mViewport.TopLeftY = 0.0f;
		mViewport.Width = (FLOAT)Width;
		mViewport.Height = (FLOAT)Height;
		mViewport.MinDepth = 0.0f;
		mViewport.MaxDepth = 1.0f;

		mScissorRect = { 0, 0, (int)(mViewport.Width), (int)(mViewport.Height) };
	}

	GRtvProperties mProperties;
	
	//ID3D12Resource* mRtvResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> mResource;

	D3D12_CPU_DESCRIPTOR_HANDLE mRtvCpu;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mSrvCpu;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mSrvGpu;

	UINT mClientWidth = 0;
	UINT mClientHeight = 0;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	bool bScaledByViewport = true;
};

