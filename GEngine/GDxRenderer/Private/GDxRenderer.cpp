#include "stdafx.h"
#include "GDxRenderer.h"
#include "GDxRendererFactory.h"
#include "GDxTexture.h"
#include "GDxFloat4.h"
#include "GDxFloat4x4.h"
#include "GDxFilmboxManager.h"
#include "GDxMesh.h"
#include "GDxSceneObject.h"
#include "GDxInputLayout.h"
#include "GDxShaderManager.h"
#include "GDxStaticVIBuffer.h"

#include <WindowsX.h>

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;


#pragma region Class

GDxRenderer& GDxRenderer::GetRenderer()
{
	static GDxRenderer *instance = new GDxRenderer();
	return *instance;
}

GDxRenderer::GDxRenderer()
{
	// Estimate the scene bounding sphere manually since we know how the scene was constructed.
	// The grid is the "widest object" with a width of 20 and depth of 30.0f, and centered at
	// the world space origin.  In general, you need to loop over every world space vertex
	// position and compute the bounding sphere.
	mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mSceneBounds.Radius = sqrtf(10.0f*10.0f + 15.0f*15.0f);
}

GDxRenderer::~GDxRenderer()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

#pragma endregion

#pragma region Main

bool GDxRenderer::InitDirect3D()
{
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	// Try to create hardware device.
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice));

	// Fallback to WARP device.
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}

	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	//m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	//assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
	LogAdapters();
#endif

	CreateCommandObjects();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();

	return true;
}

void GDxRenderer::PreInitialize(HWND OutputWindow, double width, double height)
{
	mhMainWnd = OutputWindow;
	mClientWidth = (int)width;
	mClientHeight = (int)height;

	if (!InitDirect3D())
		return;

	CreateRendererFactory();
	CreateFilmboxManager();

	// Do the initial resize code.
	OnResize();

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
}

void GDxRenderer::Initialize()
{
	BuildDescriptorHeaps();
	BuildRootSignature();
	BuildFrameResources();
	BuildPSOs();

	pImgui->Initialize(MainWnd(), md3dDevice.Get(), NUM_FRAME_RESOURCES, mSrvDescriptorHeap.Get());

	CubemapPreIntegration();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();
}

void GDxRenderer::Draw(const GGiGameTimer* gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["GBuffer"].Get()));

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// G-Buffer Pass
	{
		mCommandList->RSSetViewports(1, &(mRtvHeaps["GBuffer"]->mRtv[0]->mViewport));
		mCommandList->RSSetScissorRects(1, &(mRtvHeaps["GBuffer"]->mRtv[0]->mScissorRect));

		mCommandList->SetGraphicsRootSignature(mRootSignatures["GBuffer"].Get());

		mCommandList->SetPipelineState(mPSOs["GBuffer"].Get());

		UINT objCBByteSize = GDxUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
		auto objectCB = mCurrFrameResource->ObjectCB->Resource();

		auto passCB = mCurrFrameResource->PassCB->Resource();
		mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

		//mCommandList->SetGraphicsRootDescriptorTable(2, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		mCommandList->SetGraphicsRootDescriptorTable(2, GetGpuSrv(mTextrueHeapIndex));

		matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
		mCommandList->SetGraphicsRootShaderResourceView(3, matBuffer->GetGPUVirtualAddress());

		mCommandList->OMSetStencilRef(1);

		// Indicate a state transition on the resource usage.
		for (size_t i = 0; i < mRtvHeaps["GBuffer"]->mRtv.size(); i++)
		{
			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRtvHeaps["GBuffer"]->mRtv[i]->mResource.Get(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

			// Clear the back buffer.
			DirectX::XMVECTORF32 clearColor = { mRtvHeaps["GBuffer"]->mRtv[i]->mProperties.mClearColor[0],
			mRtvHeaps["GBuffer"]->mRtv[i]->mProperties.mClearColor[1],
			mRtvHeaps["GBuffer"]->mRtv[i]->mProperties.mClearColor[2],
			mRtvHeaps["GBuffer"]->mRtv[i]->mProperties.mClearColor[3]
			};

			// WE ALREADY WROTE THE DEPTH INFO TO THE DEPTH BUFFER IN DrawNormalsAndDepth,
			// SO DO NOT CLEAR DEPTH.
			mCommandList->ClearRenderTargetView(mRtvHeaps["GBuffer"]->mRtvHeap.handleCPU((UINT)i), clearColor, 0, nullptr);
		}

		// Specify the buffers we are going to render to.
		//mCommandList->OMSetRenderTargets(mRtvHeaps["GBuffer"]->mRtvHeap.HeapDesc.NumDescriptors, &(mRtvHeaps["GBuffer"]->mRtvHeap.hCPUHeapStart), true, &DepthStencilView());
		mCommandList->OMSetRenderTargets(mRtvHeaps["GBuffer"]->mRtvHeap.HeapDesc.NumDescriptors, &(mRtvHeaps["GBuffer"]->mRtvHeap.hCPUHeapStart), true, &DepthStencilView());

		// For each render item...
		DrawSceneObjects(mCommandList.Get(), RenderLayer::Deferred, true);

		for (size_t i = 0; i < mRtvHeaps["GBuffer"]->mRtv.size(); i++)
		{
			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRtvHeaps["GBuffer"]->mRtv[i]->mResource.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
		}
	}

	// Direct Light Pass
	{
		mCommandList->RSSetViewports(1, &(mRtvHeaps["LightPass"]->mRtv[0]->mViewport));
		mCommandList->RSSetScissorRects(1, &(mRtvHeaps["LightPass"]->mRtv[0]->mScissorRect));

		mCommandList->SetGraphicsRootSignature(mRootSignatures["LightPass"].Get());

		mCommandList->SetPipelineState(mPSOs["DirectLightPass"].Get());
		
		auto lightCB = mCurrFrameResource->LightCB->Resource();
		mCommandList->SetGraphicsRootConstantBufferView(0, lightCB->GetGPUVirtualAddress());

		mCommandList->SetGraphicsRootDescriptorTable(1, mRtvHeaps["GBuffer"]->GetSrvGpuStart());

		mCommandList->SetGraphicsRootDescriptorTable(2, GetGpuSrv(mIblIndex));
		
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRtvHeaps["LightPass"]->mRtv[0]->mResource.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// Clear the back buffer.
		DirectX::XMVECTORF32 clearColor = { mRtvHeaps["LightPass"]->mRtv[0]->mProperties.mClearColor[0],
		mRtvHeaps["LightPass"]->mRtv[0]->mProperties.mClearColor[1],
		mRtvHeaps["LightPass"]->mRtv[0]->mProperties.mClearColor[2],
		mRtvHeaps["LightPass"]->mRtv[0]->mProperties.mClearColor[3]
		};

		// WE ALREADY WROTE THE DEPTH INFO TO THE DEPTH BUFFER IN DrawNormalsAndDepth,
		// SO DO NOT CLEAR DEPTH.
		mCommandList->ClearRenderTargetView(mRtvHeaps["LightPass"]->mRtvHeap.handleCPU(0), clearColor, 0, nullptr);

		// Specify the buffers we are going to render to.
		//mCommandList->OMSetRenderTargets(mRtvHeaps["GBuffer"]->mRtvHeap.HeapDesc.NumDescriptors, &(mRtvHeaps["GBuffer"]->mRtvHeap.hCPUHeapStart), true, &DepthStencilView());
		mCommandList->OMSetRenderTargets(1, &(mRtvHeaps["LightPass"]->mRtvHeap.handleCPU(0)), true, &DepthStencilView());

		// For each render item...
		DrawSceneObjects(mCommandList.Get(), RenderLayer::ScreenQuad, false);

		//mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRtvHeaps["LightPass"]->mRtv[0]->mResource.Get(),
			//D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	// Ambient Light Pass
	{
		mCommandList->RSSetViewports(1, &(mRtvHeaps["LightPass"]->mRtv[0]->mViewport));
		mCommandList->RSSetScissorRects(1, &(mRtvHeaps["LightPass"]->mRtv[0]->mScissorRect));

		mCommandList->SetGraphicsRootSignature(mRootSignatures["LightPass"].Get());

		mCommandList->SetPipelineState(mPSOs["AmbientLightPass"].Get());
		
		auto lightCB = mCurrFrameResource->LightCB->Resource();
		mCommandList->SetGraphicsRootConstantBufferView(0, lightCB->GetGPUVirtualAddress());

		mCommandList->SetGraphicsRootDescriptorTable(1, mRtvHeaps["GBuffer"]->GetSrvGpuStart());

		mCommandList->SetGraphicsRootDescriptorTable(2, GetGpuSrv(mIblIndex));
		
		//mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRtvHeaps["LightPass"]->mRtv[0]->mResource.Get(),
			//D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// Clear the back buffer.
		DirectX::XMVECTORF32 clearColor = { mRtvHeaps["LightPass"]->mRtv[0]->mProperties.mClearColor[0],
		mRtvHeaps["LightPass"]->mRtv[0]->mProperties.mClearColor[1],
		mRtvHeaps["LightPass"]->mRtv[0]->mProperties.mClearColor[2],
		mRtvHeaps["LightPass"]->mRtv[0]->mProperties.mClearColor[3]
		};

		// WE ALREADY WROTE THE DEPTH INFO TO THE DEPTH BUFFER IN DrawNormalsAndDepth,
		// SO DO NOT CLEAR DEPTH.
		//mCommandList->ClearRenderTargetView(mRtvHeaps["LightPass"]->mRtvHeap.handleCPU(0), clearColor, 0, nullptr);

		// Specify the buffers we are going to render to.
		//mCommandList->OMSetRenderTargets(mRtvHeaps["GBuffer"]->mRtvHeap.HeapDesc.NumDescriptors, &(mRtvHeaps["GBuffer"]->mRtvHeap.hCPUHeapStart), true, &DepthStencilView());
		mCommandList->OMSetRenderTargets(1, &(mRtvHeaps["LightPass"]->mRtvHeap.handleCPU(0)), true, &DepthStencilView());

		// For each render item...
		DrawSceneObjects(mCommandList.Get(), RenderLayer::ScreenQuad, false);

		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRtvHeaps["LightPass"]->mRtv[0]->mResource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	// Sky Pass
	{
		mCommandList->RSSetViewports(1, &mScreenViewport);
		mCommandList->RSSetScissorRects(1, &mScissorRect);

		mCommandList->SetGraphicsRootSignature(mRootSignatures["Sky"].Get());

		// Clear the back buffer.
		DirectX::XMVECTORF32 clearColor = { mRtvHeaps["SkyPass"]->mRtv[0]->mProperties.mClearColor[0],
		mRtvHeaps["SkyPass"]->mRtv[0]->mProperties.mClearColor[1],
		mRtvHeaps["SkyPass"]->mRtv[0]->mProperties.mClearColor[2],
		mRtvHeaps["SkyPass"]->mRtv[0]->mProperties.mClearColor[3]
		};
		mCommandList->ClearRenderTargetView(mRtvHeaps["SkyPass"]->mRtvHeap.handleCPU(0), clearColor, 0, nullptr);

		// Specify the buffers we are going to render to.
		//mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
		mCommandList->OMSetRenderTargets(1, &(mRtvHeaps["SkyPass"]->mRtvHeap.handleCPU(0)), true, &DepthStencilView());

		auto passCB = mCurrFrameResource->SkyCB->Resource();
		mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

		// Sky cubemap SRV.
		mCommandList->SetGraphicsRootDescriptorTable(2, GetGpuSrv(mSkyTexHeapIndex));
		//mCommandList->SetGraphicsRootDescriptorTable(2, GetGpuSrv(mIblIndex + 4)); //Irradiance cubemap debug.

		mCommandList->SetPipelineState(mPSOs["Sky"].Get());
		DrawSceneObjects(mCommandList.Get(), RenderLayer::Sky, true);
	}

	// Debug Pass
	bool bDrawDebugQuad = true;
	if (bDrawDebugQuad)
	{
		mCommandList->RSSetViewports(1, &mScreenViewport);
		mCommandList->RSSetScissorRects(1, &mScissorRect);

		mCommandList->SetGraphicsRootSignature(mRootSignatures["GBufferDebug"].Get());

		mCommandList->SetPipelineState(mPSOs["GBufferDebug"].Get());

		UINT objCBByteSize = GDxUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
		auto objectCB = mCurrFrameResource->ObjectCB->Resource();

		mCommandList->SetGraphicsRootDescriptorTable(1, mRtvHeaps["GBuffer"]->GetSrvGpuStart());

		matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
		mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

		mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

		// For each render item...
		DrawSceneObjects(mCommandList.Get(), RenderLayer::Debug, true);
	}

	// Post Process Pass
	{
		mCommandList->RSSetViewports(1, &mScreenViewport);
		mCommandList->RSSetScissorRects(1, &mScissorRect);

		mCommandList->SetGraphicsRootSignature(mRootSignatures["PostProcess"].Get());

		mCommandList->SetPipelineState(mPSOs["PostProcess"].Get());

		mCommandList->SetGraphicsRootDescriptorTable(0, mRtvHeaps["LightPass"]->GetSrvGpuStart());
		mCommandList->SetGraphicsRootDescriptorTable(1, mRtvHeaps["SkyPass"]->GetSrvGpuStart());

		// Specify the buffers we are going to render to.
		//mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
		mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

		// For each render item...
		DrawSceneObjects(mCommandList.Get(), RenderLayer::ScreenQuad, false);
	}

	// Immediate Mode GUI Pass
	{
		pImgui->Render(mCommandList.Get());
	}

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(1, 0)); // Present with vsync
	//ThrowIfFailed(mSwapChain->Present(0, 0)); // Present without vsync
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void GDxRenderer::Update(const GGiGameTimer* gt)
{
	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % NUM_FRAME_RESOURCES;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	//
	// Animate the lights (and hence shadows).
	//

	mLightRotationAngle += 0.1f * gt->DeltaTime();

	XMMATRIX R = XMMatrixRotationY(mLightRotationAngle);
	for (int i = 0; i < 3; ++i)
	{
		XMVECTOR lightDir = XMLoadFloat3(&mBaseLightDirections[i]);
		lightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&mRotatedLightDirections[i], lightDir);
	}

	UpdateObjectCBs(gt);
	UpdateMaterialBuffer(gt);
	UpdateShadowTransform(gt);
	UpdateMainPassCB(gt);
	UpdateSkyPassCB(gt);
	UpdateLightCB(gt);
}

void GDxRenderer::OnResize()
{
	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);

	// Flush before changing any resources.
	FlushCommandQueue();

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffer[i].Reset();
	mDepthStencilBuffer.Reset();

	// Resize the swap chain.
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount,
		mClientWidth, mClientHeight,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, DepthStencilView());

	// Transition the resource from its initial state to be used as a depth buffer.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// Execute the resize commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();

	// Update the viewport transform to cover the client area.
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };

	for (auto &rtvHeap : mRtvHeaps)
	{
		if (rtvHeap.second->mRtv.size() != 0 && rtvHeap.second->mRtv[0]->mResource != nullptr)
		{
			rtvHeap.second->OnResize(mClientWidth, mClientHeight);
		}
	}
}

#pragma endregion

#pragma region Update

void GDxRenderer::UpdateObjectCBs(const GGiGameTimer* gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : pSceneObjects)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e.second->NumFramesDirty > 0)
		{
			GDxFloat4x4* dxTrans = dynamic_cast<GDxFloat4x4*>(e.second->GetTransform());
			if (dxTrans == nullptr)
				ThrowGGiException("Cast failed from GGiFloat4x4* to GDxFloat4x4*.");

			GDxFloat4x4* dxTexTrans = dynamic_cast<GDxFloat4x4*>(e.second->GetTexTransform());
			if (dxTexTrans == nullptr)
				ThrowGGiException("Cast failed from GGiFloat4x4* to GDxFloat4x4*.");
			
			XMMATRIX renderObjectTrans = XMLoadFloat4x4(&(dxTrans->GetValue()));
			XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(renderObjectTrans), renderObjectTrans);
			XMMATRIX invTransWorld = XMMatrixTranspose(invWorld);
			//auto tempSubTrans = e->GetSubmesh().Transform;
			//XMMATRIX submeshTrans = XMLoadFloat4x4(&tempSubTrans);
			XMMATRIX texTransform = XMLoadFloat4x4(&(dxTexTrans->GetValue()));
			//auto world = submeshTrans * renderObjectTrans;
			auto world = renderObjectTrans;

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.InvTransWorld, XMMatrixTranspose(invTransWorld));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
			objConstants.MaterialIndex = e.second->GetMaterial()->MatIndex;

			currObjectCB->CopyData(e.second->GetObjIndex(), objConstants);

			// Next FrameResource need to be updated too.
			e.second->NumFramesDirty--;
		}
	}
}

void GDxRenderer::UpdateLightCB(const GGiGameTimer* gt)
{
	LightConstants lightCB;

	auto pos = pCamera->GetPosition();
	lightCB.cameraPosition = DirectX::XMFLOAT3(pos[0], pos[1], pos[2]);

	lightCB.dirLight[0].Direction[0] = 0.57735f;
	lightCB.dirLight[0].Direction[1] = -0.57735f;
	lightCB.dirLight[0].Direction[2] = -0.57735f;
	lightCB.dirLight[0].DiffuseColor[0] = 0.7f;
	lightCB.dirLight[0].DiffuseColor[1] = 0.7f;
	lightCB.dirLight[0].DiffuseColor[2] = 0.6f;
	lightCB.dirLight[0].DiffuseColor[3] = 1.0f;
	lightCB.dirLight[0].AmbientColor[0] = 0.0f;
	lightCB.dirLight[0].AmbientColor[1] = 0.0f;
	lightCB.dirLight[0].AmbientColor[2] = 0.0f;
	lightCB.dirLight[0].AmbientColor[3] = 1.0f;
	lightCB.dirLight[0].Intensity = 3.0f;

	lightCB.dirLight[1].Direction[0] = -0.57735f;
	lightCB.dirLight[1].Direction[1] = -0.57735f;
	lightCB.dirLight[1].Direction[2] = -0.57735f;
	lightCB.dirLight[1].DiffuseColor[0] = 0.3f;
	lightCB.dirLight[1].DiffuseColor[1] = 0.3f;
	lightCB.dirLight[1].DiffuseColor[2] = 0.3f;
	lightCB.dirLight[1].DiffuseColor[3] = 1.0f;
	lightCB.dirLight[1].AmbientColor[0] = 0.0f;
	lightCB.dirLight[1].AmbientColor[1] = 0.0f;
	lightCB.dirLight[1].AmbientColor[2] = 0.0f;
	lightCB.dirLight[1].AmbientColor[3] = 1.0f;
	lightCB.dirLight[1].Intensity = 3.0f;

	lightCB.dirLight[2].Direction[0] = 0.0;
	lightCB.dirLight[2].Direction[1] = -0.707f;
	lightCB.dirLight[2].Direction[2] = 0.707f;
	lightCB.dirLight[2].DiffuseColor[0] = 0.1f;
	lightCB.dirLight[2].DiffuseColor[1] = 0.1f;
	lightCB.dirLight[2].DiffuseColor[2] = 0.1f;
	lightCB.dirLight[2].DiffuseColor[3] = 1.0f;
	lightCB.dirLight[2].AmbientColor[0] = 0.0f;
	lightCB.dirLight[2].AmbientColor[1] = 0.0f;
	lightCB.dirLight[2].AmbientColor[2] = 0.0f;
	lightCB.dirLight[2].AmbientColor[3] = 1.0f;
	lightCB.dirLight[2].Intensity = 3.0f;

	lightCB.pointLight[0].Color[0] = 1.0f;
	lightCB.pointLight[0].Color[1] = 1.0f;
	lightCB.pointLight[0].Color[2] = 1.0f;
	lightCB.pointLight[0].Color[3] = 1.0f;
	lightCB.pointLight[0].Intensity = 100.0f;
	lightCB.pointLight[0].Position[0] = 0.0f;
	lightCB.pointLight[0].Position[1] = -4.0f;
	lightCB.pointLight[0].Position[2] = 0.0f;
	lightCB.pointLight[0].Range = 100.0f;

	lightCB.dirLightCount = 3;
	lightCB.pointLightCount = 0;

	auto LightCB = mCurrFrameResource->LightCB.get();
	LightCB->CopyData(0, lightCB);
}

void GDxRenderer::UpdateMaterialBuffer(const GGiGameTimer* gt)
{
	auto currMaterialBuffer = mCurrFrameResource->MaterialBuffer.get();
	for (auto& e : pMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		GRiMaterial* mat = e.second;
		if (mat->NumFramesDirty > 0)
		{
			MaterialData matData;
			int i;
			XMMATRIX matTransform = DirectX::XMMatrixScaling(mat->GetScaleX(), mat->GetScaleY(), 1.0f);
			//GGiFloat4x4* ggiMat = mat->MatTransform.get();
			//GDxFloat4x4* dxMat = dynamic_cast<GDxFloat4x4*>(ggiMat);
			//if (dxMat == nullptr)
				//ThrowDxException(L"Dynamic cast from GRiFloat4x4 to GDxFloat4x4 failed.");
			//XMMATRIX matTransform = XMLoadFloat4x4(&dxMat->GetValue());
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));

			size_t texNum = mat->GetTextureNum();
			if (texNum > MATERIAL_MAX_TEXTURE_NUM)
				ThrowDxException(L"Material (CBIndex : " + std::to_wstring(mat->MatIndex) + L" ) texture number exceeds MATERIAL_MAX_TEXTURE_NUM.");
			for (i = 0; i < texNum; i++)
			{
				auto texName = mat->GetTextureUniqueNameByIndex(i);
				if (pTextures.find(texName) == pTextures.end())
					ThrowGGiException(L"Texture" + texName + L" not found.");
				matData.TextureIndex[i] = pTextures[texName]->texIndex;
			}

			size_t scalarNum = mat->GetScalarNum();
			if (scalarNum > MATERIAL_MAX_SCALAR_NUM)
				ThrowDxException(L"Material (CBIndex : " + std::to_wstring(mat->MatIndex) + L" ) scalar number exceeds MATERIAL_MAX_SCALAR_NUM.");
			for (i = 0; i < scalarNum; i++)
			{
				matData.ScalarParams[i] = mat->GetScalar(i);
			}

			size_t vectorNum = mat->GetVectorNum();
			if (vectorNum > MATERIAL_MAX_VECTOR_NUM)
				ThrowDxException(L"Material (CBIndex : " + std::to_wstring(mat->MatIndex) + L" ) vector number exceeds MATERIAL_MAX_VECTOR_NUM.");
			for (i = 0; i < vectorNum; i++)
			{
				GGiFloat4 ggiVec = mat->GetVector(i);
				GDxFloat4 dxVec = dynamic_cast<GDxFloat4&>(ggiVec);
				matData.VectorParams[i] = dxVec.GetValue();
			}
			//matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
			//matData.NormalMapIndex = mat->NormalSrvHeapIndex;
			//matData.Roughness = mat->Roughness;
			//matData.DiffuseAlbedo = mat->DiffuseAlbedo;
			//matData.FresnelR0 = mat->FresnelR0;

			currMaterialBuffer->CopyData(mat->MatIndex, matData);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void GDxRenderer::UpdateShadowTransform(const GGiGameTimer* gt)
{
	// Only the first "main" light casts a shadow.
	XMVECTOR lightDir = XMLoadFloat3(&mRotatedLightDirections[0]);
	XMVECTOR lightPos = -2.0f*mSceneBounds.Radius*lightDir;
	XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	XMStoreFloat3(&mLightPosW, lightPos);

	// Transform bounding sphere to light space.
	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

	// Ortho frustum in light space encloses scene.
	float l = sphereCenterLS.x - mSceneBounds.Radius;
	float b = sphereCenterLS.y - mSceneBounds.Radius;
	float n = sphereCenterLS.z - mSceneBounds.Radius;
	float r = sphereCenterLS.x + mSceneBounds.Radius;
	float t = sphereCenterLS.y + mSceneBounds.Radius;
	float f = sphereCenterLS.z + mSceneBounds.Radius;

	mLightNearZ = n;
	mLightFarZ = f;
	XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX S = lightView * lightProj*T;
	XMStoreFloat4x4(&mLightView, lightView);
	XMStoreFloat4x4(&mLightProj, lightProj);
	XMStoreFloat4x4(&mShadowTransform, S);
}

void GDxRenderer::UpdateMainPassCB(const GGiGameTimer* gt)
{
	auto viewMat = dynamic_cast<GDxFloat4x4*>(pCamera->GetView());
	if (viewMat == nullptr)
		ThrowGGiException("Cast failed from GGiFloat4x4* to GDxFloat4x4*.");

	auto projMat = dynamic_cast<GDxFloat4x4*>(pCamera->GetProj());
	if (projMat == nullptr)
		ThrowGGiException("Cast failed from GGiFloat4x4* to GDxFloat4x4*.");

	XMMATRIX view = DirectX::XMLoadFloat4x4(&(viewMat->GetValue()));
	XMMATRIX proj = DirectX::XMLoadFloat4x4(&(projMat->GetValue()));

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX viewProjTex = XMMatrixMultiply(viewProj, T);
	XMMATRIX shadowTransform = XMLoadFloat4x4(&mShadowTransform);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProjTex, XMMatrixTranspose(viewProjTex));
	XMStoreFloat4x4(&mMainPassCB.ShadowTransform, XMMatrixTranspose(shadowTransform));
	auto eyePos = pCamera->GetPosition();
	mMainPassCB.EyePosW = DirectX::XMFLOAT3(eyePos[0], eyePos[1], eyePos[2]);
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt->TotalTime();
	mMainPassCB.DeltaTime = gt->DeltaTime();
	mMainPassCB.AmbientLight = { 0.4f, 0.4f, 0.6f, 1.0f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void GDxRenderer::UpdateSkyPassCB(const GGiGameTimer* gt)
{
	auto viewMat = dynamic_cast<GDxFloat4x4*>(pCamera->GetView());
	if (viewMat == nullptr)
		ThrowGGiException("Cast failed from GGiFloat4x4* to GDxFloat4x4*.");

	auto projMat = dynamic_cast<GDxFloat4x4*>(pCamera->GetProj());
	if (projMat == nullptr)
		ThrowGGiException("Cast failed from GGiFloat4x4* to GDxFloat4x4*.");

	XMMATRIX view = DirectX::XMLoadFloat4x4(&(viewMat->GetValue()));
	XMMATRIX proj = DirectX::XMLoadFloat4x4(&(projMat->GetValue()));
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMStoreFloat4x4(&mSkyPassCB.ViewProj, XMMatrixTranspose(viewProj));
	auto eyePos = pCamera->GetPosition();
	mSkyPassCB.EyePosW = DirectX::XMFLOAT3(eyePos[0], eyePos[1], eyePos[2]);
	mSkyPassCB.roughness = 0.3f; // doesn't matter

	auto currPassCB = mCurrFrameResource->SkyCB.get();
	currPassCB->CopyData(0, mSkyPassCB);
}

#pragma endregion

#pragma region Initialization

void GDxRenderer::SetImgui(GRiImgui* imguiPtr)
{
	GDxImgui* dxImgui = dynamic_cast<GDxImgui*>(imguiPtr);
	if (dxImgui == nullptr)
		ThrowGGiException("Cast failed from GRiImgui* to GDxImgui*.");
	pImgui = dxImgui;
}

void GDxRenderer::BuildRootSignature()
{
	// GBuffer root signature
	{
		//G-Buffer inputs
		CD3DX12_DESCRIPTOR_RANGE range;
		range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_TEXTURE_NUM, 0);

		CD3DX12_ROOT_PARAMETER gBufferRootParameters[4];
		gBufferRootParameters[0].InitAsConstantBufferView(0);
		gBufferRootParameters[1].InitAsConstantBufferView(1);
		gBufferRootParameters[2].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_ALL);
		gBufferRootParameters[3].InitAsShaderResourceView(0, 1);

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, gBufferRootParameters,
			0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[2];
		StaticSamplers[0].Init(0, D3D12_FILTER_ANISOTROPIC);
		StaticSamplers[1].Init(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			0.f, 16u, D3D12_COMPARISON_FUNC_LESS_EQUAL);
		rootSigDesc.NumStaticSamplers = 2;
		rootSigDesc.pStaticSamplers = StaticSamplers;

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(md3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(mRootSignatures["GBuffer"].GetAddressOf())));
	}

	// GBufferDebug root signature
	{

		//G-Buffer inputs
		CD3DX12_DESCRIPTOR_RANGE range;
		range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, mRtvHeaps["GBuffer"]->mRtvHeap.HeapDesc.NumDescriptors, 0);

		CD3DX12_ROOT_PARAMETER gBufferDebugRootParameters[3];
		gBufferDebugRootParameters[0].InitAsConstantBufferView(0);
		gBufferDebugRootParameters[1].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_ALL);
		gBufferDebugRootParameters[2].InitAsShaderResourceView(0, 1);

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, gBufferDebugRootParameters,
			0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[2];
		StaticSamplers[0].Init(0, D3D12_FILTER_ANISOTROPIC);
		StaticSamplers[1].Init(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			0.f, 16u, D3D12_COMPARISON_FUNC_LESS_EQUAL);
		rootSigDesc.NumStaticSamplers = 2;
		rootSigDesc.pStaticSamplers = StaticSamplers;

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(md3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(mRootSignatures["GBufferDebug"].GetAddressOf())));
	}

	// Light pass signature
	{
		//G-Buffer inputs
		CD3DX12_DESCRIPTOR_RANGE range;
		range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, mRtvHeaps["GBuffer"]->mRtvHeap.HeapDesc.NumDescriptors, 0);

		//IBL inputs
		CD3DX12_DESCRIPTOR_RANGE rangeIBL;
		rangeIBL.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)mPrefilterLevels + (UINT)1 + (UINT)1, mRtvHeaps["GBuffer"]->mRtvHeap.HeapDesc.NumDescriptors);

		CD3DX12_ROOT_PARAMETER gLightPassRootParameters[3];
		gLightPassRootParameters[0].InitAsConstantBufferView(0);
		gLightPassRootParameters[1].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_ALL);
		gLightPassRootParameters[2].InitAsDescriptorTable(1, &rangeIBL, D3D12_SHADER_VISIBILITY_ALL);

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, gLightPassRootParameters,
			0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[2];
		StaticSamplers[0].Init(0, D3D12_FILTER_ANISOTROPIC);
		StaticSamplers[1].Init(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			0.f, 16u, D3D12_COMPARISON_FUNC_LESS_EQUAL);
		rootSigDesc.NumStaticSamplers = 2;
		rootSigDesc.pStaticSamplers = StaticSamplers;

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(md3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(mRootSignatures["LightPass"].GetAddressOf())));
	}

	// Post process signature
	{
		//G-Buffer inputs
		CD3DX12_DESCRIPTOR_RANGE lightRange;
		lightRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, mRtvHeaps["LightPass"]->mRtvHeap.HeapDesc.NumDescriptors, 0);
		CD3DX12_DESCRIPTOR_RANGE skyRange;
		skyRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, mRtvHeaps["LightPass"]->mRtvHeap.HeapDesc.NumDescriptors, 1);

		CD3DX12_ROOT_PARAMETER gPostProcessRootParameters[2];
		gPostProcessRootParameters[0].InitAsDescriptorTable(1, &lightRange, D3D12_SHADER_VISIBILITY_ALL);
		gPostProcessRootParameters[1].InitAsDescriptorTable(1, &skyRange, D3D12_SHADER_VISIBILITY_ALL);

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, gPostProcessRootParameters,
			0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[2];
		StaticSamplers[0].Init(0, D3D12_FILTER_ANISOTROPIC);
		StaticSamplers[1].Init(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			0.f, 16u, D3D12_COMPARISON_FUNC_LESS_EQUAL);
		rootSigDesc.NumStaticSamplers = 2;
		rootSigDesc.pStaticSamplers = StaticSamplers;

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(md3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(mRootSignatures["PostProcess"].GetAddressOf())));
	}

	// Sky root signature
	{
		CD3DX12_DESCRIPTOR_RANGE texTable0;
		texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[3];

		// Perfomance TIP: Order from most frequent to least frequent.
		slotRootParameter[0].InitAsConstantBufferView(0);
		slotRootParameter[1].InitAsConstantBufferView(1);
		slotRootParameter[2].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_ALL);

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter,
			0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[2];
		StaticSamplers[0].Init(0, D3D12_FILTER_ANISOTROPIC);
		StaticSamplers[1].Init(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			0.f, 16u, D3D12_COMPARISON_FUNC_LESS_EQUAL);
		rootSigDesc.NumStaticSamplers = 2;
		rootSigDesc.pStaticSamplers = StaticSamplers;

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		//ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(md3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(mRootSignatures["Sky"].GetAddressOf())));
	}

	// Forward root signature
	{
		CD3DX12_DESCRIPTOR_RANGE texTable0;
		texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0);

		CD3DX12_DESCRIPTOR_RANGE texTable1;
		texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)pTextures.size(), 3, 0);//10,3,0

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[5];

		// Perfomance TIP: Order from most frequent to least frequent.
		slotRootParameter[0].InitAsConstantBufferView(0);
		slotRootParameter[1].InitAsConstantBufferView(1);
		slotRootParameter[2].InitAsShaderResourceView(0, 1);
		slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
		slotRootParameter[4].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

		auto staticSamplers = GetStaticSamplers();

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter,
			(UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		//ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(md3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(mRootSignatures["Forward"].GetAddressOf())));
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GDxRenderer::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC shadow(
		6, // shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                               // mipLODBias
		16,                                 // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp,
		shadow
	};
}

void GDxRenderer::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = MAX_TEXTURE_NUM
		+ 1 //imgui
		+ 1 //sky
		+ 4 //g-buffer
		+ 1 //light pass
		+ 1 //sky pass
		+ (2 + mPrefilterLevels);//IBL
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));


	//
	// Fill out the heap with actual descriptors.
	//

	mSkyTexHeapIndex = 1; // 0 is for imgui.

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	//auto skyCubeMap = mTextures["skyCubeMap"]->Resource;
	D3D12_SHADER_RESOURCE_VIEW_DESC skySrvDesc = {};
	skySrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	skySrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	skySrvDesc.TextureCube.MostDetailedMip = 0;
	GDxTexture* tex = dynamic_cast<GDxTexture*>(pTextures[L"skyCubeMap"]);
	if (tex == nullptr)
		ThrowDxException(L"Dynamic cast from GRiTexture to GDxTexture failed.");
	skySrvDesc.TextureCube.MipLevels = tex->Resource->GetDesc().MipLevels;
	skySrvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	skySrvDesc.Format = tex->Resource->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(tex->Resource.Get(), &skySrvDesc, GetCpuSrv(mSkyTexHeapIndex));

	// Build RTV heap and SRV for GBuffers.
	{
		mGBufferSrvIndex = mSkyTexHeapIndex + 1;

		std::vector<DXGI_FORMAT> rtvFormats =
		{
			DXGI_FORMAT_R32G32B32A32_FLOAT,//Albedo
			DXGI_FORMAT_R8G8B8A8_SNORM, //Normal
			DXGI_FORMAT_R32G32B32A32_FLOAT, //WorldPos
			DXGI_FORMAT_R8G8B8A8_UNORM, //OcclusionRoughnessMetallic
		};
		std::vector<std::vector<FLOAT>> rtvClearColor =
		{
			{ 0,0,0,0 },
			{ 0,0,0,0 },
			{ 0,0,0,0 },
			{ 0,0.3f,0,0 }
		};
		std::vector<GRtvProperties> propVec;
		for (size_t i = 0; i < rtvFormats.size(); i++)
		{
			GRtvProperties prop;
			prop.mRtvFormat = rtvFormats[i];
			prop.mClearColor[0] = rtvClearColor[i][0];
			prop.mClearColor[1] = rtvClearColor[i][1];
			prop.mClearColor[2] = rtvClearColor[i][2];
			prop.mClearColor[3] = rtvClearColor[i][3];
			propVec.push_back(prop);
		}
		auto gBufferRtvHeap = std::make_unique<GDxRtvHeap>(md3dDevice.Get(), mClientWidth, mClientHeight, GetCpuSrv(mGBufferSrvIndex), GetGpuSrv(mGBufferSrvIndex), propVec);
		mRtvHeaps["GBuffer"] = std::move(gBufferRtvHeap);
	}

	// Build RTV heap and SRV for light pass.
	{
		mLightPassSrvIndex = mGBufferSrvIndex + mRtvHeaps["GBuffer"]->mRtvHeap.HeapDesc.NumDescriptors;

		std::vector<DXGI_FORMAT> rtvFormats =
		{
			DXGI_FORMAT_R32G32B32A32_FLOAT// Direct light and ambient light
		};
		std::vector<std::vector<FLOAT>> rtvClearColor =
		{
			{ 0,0,0,1 }
		};
		std::vector<GRtvProperties> propVec;
		for (auto i = 0u; i < rtvFormats.size(); i++)
		{
			GRtvProperties prop;
			prop.mRtvFormat = rtvFormats[i];
			prop.mClearColor[0] = rtvClearColor[i][0];
			prop.mClearColor[1] = rtvClearColor[i][1];
			prop.mClearColor[2] = rtvClearColor[i][2];
			prop.mClearColor[3] = rtvClearColor[i][3];
			propVec.push_back(prop);
		}
		auto lightPassRtvHeap = std::make_unique<GDxRtvHeap>(md3dDevice.Get(), mClientWidth, mClientHeight, GetCpuSrv(mLightPassSrvIndex), GetGpuSrv(mLightPassSrvIndex), propVec);
		mRtvHeaps["LightPass"] = std::move(lightPassRtvHeap);
	}

	// Build RTV heap and SRV for sky pass.
	{
		mSkyPassSrvIndex = mLightPassSrvIndex + mRtvHeaps["LightPass"]->mRtvHeap.HeapDesc.NumDescriptors;

		std::vector<DXGI_FORMAT> rtvFormats =
		{
			//DXGI_FORMAT_R32G32B32A32_FLOAT
			DXGI_FORMAT_R8G8B8A8_UNORM// sky
		};
		std::vector<std::vector<FLOAT>> rtvClearColor =
		{
			{ 0,0,0,0 }
		};
		std::vector<GRtvProperties> propVec;
		for (auto i = 0u; i < rtvFormats.size(); i++)
		{
			GRtvProperties prop;
			prop.mRtvFormat = rtvFormats[i];
			prop.mClearColor[0] = rtvClearColor[i][0];
			prop.mClearColor[1] = rtvClearColor[i][1];
			prop.mClearColor[2] = rtvClearColor[i][2];
			prop.mClearColor[3] = rtvClearColor[i][3];
			propVec.push_back(prop);
		}
		auto skyPassRtvHeap = std::make_unique<GDxRtvHeap>(md3dDevice.Get(), mClientWidth, mClientHeight, GetCpuSrv(mSkyPassSrvIndex), GetGpuSrv(mSkyPassSrvIndex), propVec);
		mRtvHeaps["SkyPass"] = std::move(skyPassRtvHeap);
	}

	// Build cubemap SRV and RTVs for irradiance pre-integration.
	{
		mIblIndex = mSkyPassSrvIndex + 1;

		GRtvProperties prop;
		prop.mRtvFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		prop.mClearColor[0] = 0;
		prop.mClearColor[1] = 0;
		prop.mClearColor[2] = 0;
		prop.mClearColor[3] = 1;

		auto gIrradianceCubemap = std::make_unique<GDxCubeRtv>(md3dDevice.Get(), 2048, GetCpuSrv(mIblIndex), GetGpuSrv(mIblIndex), prop);
		mCubeRtvs["Irradiance"] = std::move(gIrradianceCubemap);
	}

	// Build SRV for LUT
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		GDxTexture* tex = dynamic_cast<GDxTexture*>(pTextures[L"Resource\\Textures\\IBL_BRDF_LUT.png"]);
		if (tex == nullptr)
			ThrowDxException(L"Dynamic cast from GRiTexture to GDxTexture failed.");

		srvDesc.Format = tex->Resource->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = tex->Resource->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(tex->Resource.Get(), &srvDesc, GetCpuSrv(mIblIndex + 1));
	}

	// Build cubemap SRV and RTVs for prefilter cubemap pre-integration.
	{
		for (auto i = 0u; i < mPrefilterLevels; i++)
		{
			GRtvProperties prop;
			prop.mRtvFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
			prop.mClearColor[0] = 0;
			prop.mClearColor[1] = 0;
			prop.mClearColor[2] = 0;
			prop.mClearColor[3] = 1;

			auto gPrefilterCubemap = std::make_unique<GDxCubeRtv>(md3dDevice.Get(), (UINT)(2048 / pow(2, i)), GetCpuSrv(mIblIndex + 2 + i), GetGpuSrv(mIblIndex + 2 + i), prop);
			mCubeRtvs["Prefilter_" + std::to_string(i)] = std::move(gPrefilterCubemap);
		}
	}

	// Build SRV for ordinary textures.
	{
		mTextrueHeapIndex = mIblIndex + 2 + mPrefilterLevels;

		for (auto i = 0u; i < MAX_TEXTURE_NUM; i++)
			mTexturePoolFreeIndex.push_back(i);

		for (auto tex : pTextures)
		{
			RegisterTexture(tex.second);
		}
	}
}

void GDxRenderer::BuildPSOs()
{
	//
	// PSO for GBuffers.
	//
	D3D12_DEPTH_STENCIL_DESC gBufferDSD;
	gBufferDSD.DepthEnable = true;
	gBufferDSD.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gBufferDSD.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gBufferDSD.StencilEnable = true;
	gBufferDSD.StencilReadMask = 0xff;
	gBufferDSD.StencilWriteMask = 0xff;
	gBufferDSD.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	gBufferDSD.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	gBufferDSD.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	gBufferDSD.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	// We are not rendering backfacing polygons, so these settings do not matter. 
	gBufferDSD.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	gBufferDSD.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	gBufferDSD.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	gBufferDSD.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gBufferPsoDesc;
	ZeroMemory(&gBufferPsoDesc, sizeof(gBufferPsoDesc));
	gBufferPsoDesc.VS = GDxShaderManager::LoadShader(L"Shaders\\DefaultVS.cso");
	gBufferPsoDesc.PS = GDxShaderManager::LoadShader(L"Shaders\\DeferredPS.cso");
	gBufferPsoDesc.InputLayout.pInputElementDescs = GDxInputLayout::DefaultLayout;
	gBufferPsoDesc.InputLayout.NumElements = _countof(GDxInputLayout::DefaultLayout);
	gBufferPsoDesc.pRootSignature = mRootSignatures["GBuffer"].Get();
	//gBufferPsoDesc.pRootSignature = mRootSignatures["Forward"].Get();
	gBufferPsoDesc.DepthStencilState = gBufferDSD;
	gBufferPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gBufferPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gBufferPsoDesc.SampleMask = UINT_MAX;
	gBufferPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gBufferPsoDesc.NumRenderTargets = (UINT)mRtvHeaps["GBuffer"]->mRtv.size();
	for (size_t i = 0; i < mRtvHeaps["GBuffer"]->mRtv.size(); i++)
	{
		gBufferPsoDesc.RTVFormats[i] = mRtvHeaps["GBuffer"]->mRtv[i]->mProperties.mRtvFormat;
	}
	gBufferPsoDesc.DSVFormat = mDepthStencilFormat;
	gBufferPsoDesc.SampleDesc.Count = 1;//can't use msaa in deferred rendering.
	//deferredPSO = sysRM->CreatePSO(StringID("deferredPSO"), descPipelineState);
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&gBufferPsoDesc, IID_PPV_ARGS(&mPSOs["GBuffer"])));

	//
	// PSO for direct light pass.
	//
	D3D12_DEPTH_STENCIL_DESC lightPassDSD;
	lightPassDSD.DepthEnable = false;
	lightPassDSD.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	lightPassDSD.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	lightPassDSD.StencilEnable = true;
	lightPassDSD.StencilReadMask = 0xff;
	lightPassDSD.StencilWriteMask = 0x0;
	lightPassDSD.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	lightPassDSD.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	lightPassDSD.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	lightPassDSD.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	// We are not rendering backfacing polygons, so these settings do not matter. 
	lightPassDSD.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	lightPassDSD.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	lightPassDSD.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	lightPassDSD.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	auto blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	blendState.AlphaToCoverageEnable = false;
	blendState.IndependentBlendEnable = false;

	blendState.RenderTarget[0].BlendEnable = true;
	blendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	blendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

	auto rasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//rasterizer.CullMode = D3D12_CULL_MODE_FRONT; // Front culling for point light
	rasterizer.CullMode = D3D12_CULL_MODE_NONE;
	rasterizer.DepthClipEnable = false;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState;
	ZeroMemory(&descPipelineState, sizeof(descPipelineState));

	descPipelineState.VS = GDxShaderManager::LoadShader(L"Shaders\\FullScreenVS.cso");
	descPipelineState.PS = GDxShaderManager::LoadShader(L"Shaders\\DirectLightPassPS.cso");
	descPipelineState.pRootSignature = mRootSignatures["LightPass"].Get();
	descPipelineState.BlendState = blendState;
	descPipelineState.DepthStencilState = lightPassDSD;
	descPipelineState.DepthStencilState.DepthEnable = false;
	descPipelineState.InputLayout.pInputElementDescs = GDxInputLayout::DefaultLayout;
	descPipelineState.InputLayout.NumElements = _countof(GDxInputLayout::DefaultLayout);
	descPipelineState.RasterizerState = rasterizer;
	descPipelineState.NumRenderTargets = 1;
	descPipelineState.RTVFormats[0] = mRtvHeaps["LightPass"]->mRtv[0]->mProperties.mRtvFormat;
	descPipelineState.SampleMask = UINT_MAX;
	descPipelineState.SampleDesc.Count = 1;
	descPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&descPipelineState, IID_PPV_ARGS(&mPSOs["DirectLightPass"])));

	//
	// PSO for ambient light pass.
	//
	D3D12_DEPTH_STENCIL_DESC ambientPassDSD;
	ambientPassDSD.DepthEnable = false;
	ambientPassDSD.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ambientPassDSD.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	ambientPassDSD.StencilEnable = true;
	ambientPassDSD.StencilReadMask = 0xff;
	ambientPassDSD.StencilWriteMask = 0x0;
	ambientPassDSD.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	ambientPassDSD.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	ambientPassDSD.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	ambientPassDSD.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	// We are not rendering backfacing polygons, so these settings do not matter. 
	ambientPassDSD.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	ambientPassDSD.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	ambientPassDSD.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	ambientPassDSD.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	auto ambientBlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	ambientBlendState.AlphaToCoverageEnable = false;
	ambientBlendState.IndependentBlendEnable = false;

	ambientBlendState.RenderTarget[0].BlendEnable = true;
	ambientBlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	ambientBlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	ambientBlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

	auto ambientRasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//rasterizer.CullMode = D3D12_CULL_MODE_FRONT; // Front culling for point light
	ambientRasterizer.CullMode = D3D12_CULL_MODE_NONE;
	ambientRasterizer.DepthClipEnable = false;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC descAmbientPSO;
	ZeroMemory(&descAmbientPSO, sizeof(descAmbientPSO));

	descAmbientPSO.VS = GDxShaderManager::LoadShader(L"Shaders\\FullScreenVS.cso");
	descAmbientPSO.PS = GDxShaderManager::LoadShader(L"Shaders\\AmbientPassPS.cso");
	descAmbientPSO.pRootSignature = mRootSignatures["LightPass"].Get();
	descAmbientPSO.BlendState = ambientBlendState;
	descAmbientPSO.DepthStencilState = ambientPassDSD;
	descAmbientPSO.DepthStencilState.DepthEnable = false;
	descAmbientPSO.InputLayout.pInputElementDescs = GDxInputLayout::DefaultLayout;
	descAmbientPSO.InputLayout.NumElements = _countof(GDxInputLayout::DefaultLayout);
	descAmbientPSO.RasterizerState = ambientRasterizer;
	descAmbientPSO.NumRenderTargets = 1;
	descAmbientPSO.RTVFormats[0] = mRtvHeaps["LightPass"]->mRtv[0]->mProperties.mRtvFormat;
	descAmbientPSO.SampleMask = UINT_MAX;
	descAmbientPSO.SampleDesc.Count = 1;
	descAmbientPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&descAmbientPSO, IID_PPV_ARGS(&mPSOs["AmbientLightPass"])));

	//
	// PSO for post process.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PostProcessPsoDesc;

	D3D12_DEPTH_STENCIL_DESC postProcessDSD;
	postProcessDSD.DepthEnable = true;
	postProcessDSD.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	postProcessDSD.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	postProcessDSD.StencilEnable = false;
	postProcessDSD.StencilReadMask = 0xff;
	postProcessDSD.StencilWriteMask = 0x0;
	postProcessDSD.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	postProcessDSD.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	postProcessDSD.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	postProcessDSD.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	// We are not rendering backfacing polygons, so these settings do not matter. 
	postProcessDSD.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	postProcessDSD.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	postProcessDSD.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	postProcessDSD.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

	ZeroMemory(&PostProcessPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	PostProcessPsoDesc.InputLayout.pInputElementDescs = GDxInputLayout::DefaultLayout;
	PostProcessPsoDesc.InputLayout.NumElements = _countof(GDxInputLayout::DefaultLayout);
	PostProcessPsoDesc.pRootSignature = mRootSignatures["PostProcess"].Get();
	PostProcessPsoDesc.VS = GDxShaderManager::LoadShader(L"Shaders\\FullScreenVS.cso");
	PostProcessPsoDesc.PS = GDxShaderManager::LoadShader(L"Shaders\\PostProcessPS.cso");
	PostProcessPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	PostProcessPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PostProcessPsoDesc.DepthStencilState = postProcessDSD;
	PostProcessPsoDesc.SampleMask = UINT_MAX;
	PostProcessPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PostProcessPsoDesc.NumRenderTargets = 1;
	PostProcessPsoDesc.RTVFormats[0] = mBackBufferFormat;
	PostProcessPsoDesc.SampleDesc.Count = 1;//m4xMsaaState ? 4 : 1;
	PostProcessPsoDesc.SampleDesc.Quality = 0;//m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	PostProcessPsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&PostProcessPsoDesc, IID_PPV_ARGS(&mPSOs["PostProcess"])));


	//
	// PSO for GBuffer debug layer.
	//
	D3D12_DEPTH_STENCIL_DESC gBufferDebugDSD;
	gBufferDebugDSD.DepthEnable = true;
	gBufferDebugDSD.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gBufferDebugDSD.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gBufferDebugDSD.StencilEnable = false;
	gBufferDebugDSD.StencilReadMask = 0xff;
	gBufferDebugDSD.StencilWriteMask = 0x0;
	gBufferDebugDSD.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	gBufferDebugDSD.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	gBufferDebugDSD.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	gBufferDebugDSD.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	// We are not rendering backfacing polygons, so these settings do not matter. 
	gBufferDebugDSD.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	gBufferDebugDSD.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	gBufferDebugDSD.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	gBufferDebugDSD.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPsoDesc;

	ZeroMemory(&debugPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	debugPsoDesc.InputLayout.pInputElementDescs = GDxInputLayout::DefaultLayout;
	debugPsoDesc.InputLayout.NumElements = _countof(GDxInputLayout::DefaultLayout);
	debugPsoDesc.pRootSignature = mRootSignatures["Forward"].Get();
	debugPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	debugPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	debugPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	debugPsoDesc.SampleMask = UINT_MAX;
	debugPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	debugPsoDesc.NumRenderTargets = 1;
	debugPsoDesc.RTVFormats[0] = mBackBufferFormat;
	debugPsoDesc.SampleDesc.Count = 1;
	debugPsoDesc.SampleDesc.Quality = 0;
	debugPsoDesc.DSVFormat = mDepthStencilFormat;
	debugPsoDesc.pRootSignature = mRootSignatures["GBufferDebug"].Get();
	debugPsoDesc.VS = GDxShaderManager::LoadShader(L"Shaders\\ScreenVS.cso");
	debugPsoDesc.PS = GDxShaderManager::LoadShader(L"Shaders\\GBufferDebugPS.cso");
	debugPsoDesc.DepthStencilState = gBufferDebugDSD;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&debugPsoDesc, IID_PPV_ARGS(&mPSOs["GBufferDebug"])));

	//
	// PSO for sky.
	//

	D3D12_DEPTH_STENCIL_DESC gskyDSD;
	gskyDSD.DepthEnable = true;
	gskyDSD.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gskyDSD.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	gskyDSD.StencilEnable = false;
	gskyDSD.StencilReadMask = 0xff;
	gskyDSD.StencilWriteMask = 0x0;
	gskyDSD.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	gskyDSD.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	gskyDSD.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	gskyDSD.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	gskyDSD.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	gskyDSD.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	gskyDSD.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	gskyDSD.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc;

	ZeroMemory(&skyPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	skyPsoDesc.InputLayout.pInputElementDescs = GDxInputLayout::DefaultLayout;
	skyPsoDesc.InputLayout.NumElements = _countof(GDxInputLayout::DefaultLayout);
	skyPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	skyPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	//skyPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	//skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	//skyPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	skyPsoDesc.DepthStencilState = gskyDSD;
	skyPsoDesc.SampleMask = UINT_MAX;
	skyPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	skyPsoDesc.NumRenderTargets = 1;
	skyPsoDesc.RTVFormats[0] = mRtvHeaps["SkyPass"]->mRtv[0]->mProperties.mRtvFormat;;
	skyPsoDesc.SampleDesc.Count = 1;
	skyPsoDesc.SampleDesc.Quality = 0;
	skyPsoDesc.DSVFormat = mDepthStencilFormat;

	// The camera is inside the sky sphere, so just turn off culling.
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// Make sure the depth function is LESS_EQUAL and not just LESS.  
	// Otherwise, the normalized depth values at z = 1 (NDC) will 
	// fail the depth test if the depth buffer was cleared to 1.
	skyPsoDesc.pRootSignature = mRootSignatures["Sky"].Get();
	skyPsoDesc.VS = GDxShaderManager::LoadShader(L"Shaders\\SkyVS.cso");
	skyPsoDesc.PS = GDxShaderManager::LoadShader(L"Shaders\\SkyPS.cso");
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs["Sky"])));

	//
	// PSO for irradiance pre-integration.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC irradiancePsoDesc;

	ZeroMemory(&irradiancePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	irradiancePsoDesc.InputLayout.pInputElementDescs = GDxInputLayout::DefaultLayout;
	irradiancePsoDesc.InputLayout.NumElements = _countof(GDxInputLayout::DefaultLayout);
	irradiancePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	irradiancePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	irradiancePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	irradiancePsoDesc.SampleMask = UINT_MAX;
	irradiancePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	irradiancePsoDesc.NumRenderTargets = 1;
	irradiancePsoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	irradiancePsoDesc.SampleDesc.Count = 1;
	irradiancePsoDesc.SampleDesc.Quality = 0;
	irradiancePsoDesc.DSVFormat = mDepthStencilFormat;

	// The camera is inside the sky sphere, so just turn off culling.
	irradiancePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// Make sure the depth function is LESS_EQUAL and not just LESS.  
	// Otherwise, the normalized depth values at z = 1 (NDC) will 
	// fail the depth test if the depth buffer was cleared to 1.
	irradiancePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	irradiancePsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	irradiancePsoDesc.pRootSignature = mRootSignatures["Sky"].Get();
	irradiancePsoDesc.VS = GDxShaderManager::LoadShader(L"Shaders\\SkyVS.cso");
	irradiancePsoDesc.PS = GDxShaderManager::LoadShader(L"Shaders\\IrradianceCubemapPS.cso");
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&irradiancePsoDesc, IID_PPV_ARGS(&mPSOs["Irradiance"])));

	//
	// PSO for prefilter pre-integration.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC prefilterPsoDesc;

	ZeroMemory(&prefilterPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	prefilterPsoDesc.InputLayout.pInputElementDescs = GDxInputLayout::DefaultLayout;
	prefilterPsoDesc.InputLayout.NumElements = _countof(GDxInputLayout::DefaultLayout);
	prefilterPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	prefilterPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	prefilterPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	prefilterPsoDesc.SampleMask = UINT_MAX;
	prefilterPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	prefilterPsoDesc.NumRenderTargets = 1;
	prefilterPsoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	prefilterPsoDesc.SampleDesc.Count = 1;
	prefilterPsoDesc.SampleDesc.Quality = 0;
	prefilterPsoDesc.DSVFormat = mDepthStencilFormat;

	// The camera is inside the sky sphere, so just turn off culling.
	prefilterPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// Make sure the depth function is LESS_EQUAL and not just LESS.  
	// Otherwise, the normalized depth values at z = 1 (NDC) will 
	// fail the depth test if the depth buffer was cleared to 1.
	prefilterPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	prefilterPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	prefilterPsoDesc.pRootSignature = mRootSignatures["Sky"].Get();
	prefilterPsoDesc.VS = GDxShaderManager::LoadShader(L"Shaders\\SkyVS.cso");
	prefilterPsoDesc.PS = GDxShaderManager::LoadShader(L"Shaders\\PrefilterCubemapPS.cso");
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&prefilterPsoDesc, IID_PPV_ARGS(&mPSOs["Prefilter"])));

}

void GDxRenderer::BuildFrameResources()
{
	for (int i = 0; i < NUM_FRAME_RESOURCES; ++i)
	{
		mFrameResources.push_back(std::make_unique<GDxFrameResource>(md3dDevice.Get(),
			2, MAX_SCENE_OBJECT_NUM, MAX_MATERIAL_NUM));//(UINT)pSceneObjects.size(), (UINT)pMaterials.size()));
	}
}

void GDxRenderer::CubemapPreIntegration()
{
	for (auto i = 0u; i < (6 * mPrefilterLevels); i++)
	{

		PreIntegrationPassCbs.push_back(std::make_unique<GDxUploadBuffer<SkyPassConstants>>(md3dDevice.Get(), 1, true));
	}

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	//
	// Irradiance cubemap pre-integration
	//

	// Reset root parameters and PSO.
	mCommandList->RSSetViewports(1, &mCubeRtvs["Irradiance"]->mViewport);
	mCommandList->RSSetScissorRects(1, &mCubeRtvs["Irradiance"]->mScissorRect);
	
	mCommandList->SetGraphicsRootSignature(mRootSignatures["Sky"].Get());

	mCommandList->SetPipelineState(mPSOs["Irradiance"].Get());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeRtvs["Irradiance"]->mResource.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Load object CB.
	mCurrFrameResource = mFrameResources[0].get();
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : pSceneObjects)
	{
		GDxFloat4x4* dxTrans = dynamic_cast<GDxFloat4x4*>(e.second->GetTransform());
		if (dxTrans == nullptr)
			ThrowGGiException("Cast failed from GGiFloat4x4* to GDxFloat4x4*.");

		GDxFloat4x4* dxTexTrans = dynamic_cast<GDxFloat4x4*>(e.second->GetTexTransform());
		if (dxTexTrans == nullptr)
			ThrowGGiException("Cast failed from GGiFloat4x4* to GDxFloat4x4*.");

		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		XMMATRIX world = XMLoadFloat4x4(&(dxTrans->GetValue()));
		XMMATRIX texTransform = XMLoadFloat4x4(&(dxTexTrans->GetValue()));

		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
		objConstants.MaterialIndex = e.second->GetMaterial()->MatIndex;

		currObjectCB->CopyData(e.second->GetObjIndex(), objConstants);
	}

	// Load sky pass CB.
	for (auto i = 0u; i < mPrefilterLevels; i++)
	{
		for (auto j = 0u; j < 6u; j++)
		{
			auto view = pCubemapSampleCamera[j]->GetView();
			auto proj = pCubemapSampleCamera[j]->GetProj();
			GGiFloat4x4* viewProj = &((*proj) * (*view));
			GDxFloat4x4* dxVP = dynamic_cast<GDxFloat4x4*>(viewProj);
			if (dxVP == nullptr)
				ThrowGGiException("Cast fail from GGiFloat4x4* to GDxFloat4x4*.");

			dxVP->Transpose();
			mSkyPassCB.ViewProj = dxVP->GetValue();
			auto eyePos = pCamera->GetPosition();
			mSkyPassCB.EyePosW = DirectX::XMFLOAT3(eyePos[0], eyePos[1], eyePos[2]);
			if (i == 0)
			{
				mSkyPassCB.roughness = 0.01f;
			}
			else
			{
				mSkyPassCB.roughness = ((float)i / (float)mPrefilterLevels);
			}
			auto uploadCB = PreIntegrationPassCbs[i * 6 + j].get();
			uploadCB->CopyData(0, mSkyPassCB);
		}
	}

	for (auto i = 0u; i < 6; i++)
	{
		mCommandList->ClearRenderTargetView(mCubeRtvs["Irradiance"]->mRtvHeap.handleCPU(i), Colors::LightSteelBlue, 0, nullptr);

		auto skyCB = PreIntegrationPassCbs[i]->Resource();
		mCommandList->SetGraphicsRootConstantBufferView(1, skyCB->GetGPUVirtualAddress());

		CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		skyTexDescriptor.Offset(mSkyTexHeapIndex, mCbvSrvUavDescriptorSize);
		mCommandList->SetGraphicsRootDescriptorTable(2, skyTexDescriptor);

		mCommandList->OMSetRenderTargets(1, &(mCubeRtvs["Irradiance"]->mRtvHeap.handleCPU(i)), true, nullptr);

		DrawSceneObjects(mCommandList.Get(), RenderLayer::Sky, true);
	}

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeRtvs["Irradiance"]->mResource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

	//
	// Prefilter cubemap pre-integration
	//

	// Reset root parameters and PSO.
	mCommandList->SetGraphicsRootSignature(mRootSignatures["Sky"].Get());

	mCommandList->SetPipelineState(mPSOs["Prefilter"].Get());

	for (auto i = 0u; i < mPrefilterLevels; i++)
	{
		for (auto j = 0u; j < 6; j++)
		{
			mCommandList->RSSetViewports(1, &mCubeRtvs["Prefilter_" + std::to_string(i)]->mViewport);
			mCommandList->RSSetScissorRects(1, &mCubeRtvs["Prefilter_" + std::to_string(i)]->mScissorRect);

			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeRtvs["Prefilter_" + std::to_string(i)]->mResource.Get(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

			mCommandList->ClearRenderTargetView(mCubeRtvs["Prefilter_" + std::to_string(i)]->mRtvHeap.handleCPU(j), Colors::LightSteelBlue, 0, nullptr);

			//SetPassCbByCamera(PreIntegrationPassCB[i].get(), 0.0f, 0.0f, mCubemapSampleCamera[i]);
			auto passCB = PreIntegrationPassCbs[i * 6 + j]->Resource();
			mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

			CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			skyTexDescriptor.Offset(mSkyTexHeapIndex, mCbvSrvUavDescriptorSize);
			mCommandList->SetGraphicsRootDescriptorTable(2, skyTexDescriptor);

			mCommandList->OMSetRenderTargets(1, &(mCubeRtvs["Prefilter_" + std::to_string(i)]->mRtvHeap.handleCPU(j)), true, nullptr);

			DrawSceneObjects(mCommandList.Get(), RenderLayer::Sky, true);

			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeRtvs["Prefilter_" + std::to_string(i)]->mResource.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
		}
	}
}

/*
void GDxRenderer::SaveBakedCubemap(std::wstring workDir, std::wstring CubemapPath)
{
	std::wstring originalPath = workDir + CubemapPath;
	std::wstring savePathPrefix = originalPath.substr(0, originalPath.rfind(L"."));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeRtvs["Irradiance"]->mResource.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE));
	ThrowIfFailed(
		DirectX::SaveDDSTextureToFile(mCommandQueue.Get(),
			mCubeRtvs["Irradiance"]->mResource.Get(),
			(savePathPrefix + L"_Irradiance.dds").c_str(),
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_COPY_SOURCE)
	);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeRtvs["Irradiance"]->mResource.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ));
	
	for (auto i = 0u; i < mPrefilterLevels; i++)
	{
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeRtvs["Prefilter_" + std::to_string(i)]->mResource.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE));
		ThrowIfFailed(
			DirectX::SaveDDSTextureToFile(mCommandQueue.Get(),
				mCubeRtvs["Prefilter_" + std::to_string(i)]->mResource.Get(),
				(savePathPrefix + L"_Prefilter_" + std::to_wstring(i) + L".dds").c_str(),
				D3D12_RESOURCE_STATE_COPY_SOURCE,
				D3D12_RESOURCE_STATE_COPY_SOURCE)
		);
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeRtvs["Prefilter_" + std::to_string(i)]->mResource.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ));
	}
}
*/

void GDxRenderer::CreateRendererFactory()
{
	GDxRendererFactory fac(md3dDevice.Get(), mCommandList.Get(), mCommandQueue.Get());
	mFactory = std::make_unique<GDxRendererFactory>(fac);
}

void GDxRenderer::CreateFilmboxManager()
{
	mFilmboxManager = std::make_unique<GDxFilmboxManager>();
	mFilmboxManager->SetRendererFactory(mFactory.get());
}

void GDxRenderer::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mCommandList->Close();
}

void GDxRenderer::CreateSwapChain()
{
	// Release the previous swapchain we will be recreating.
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.OutputWindow = mhMainWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd,
		mSwapChain.GetAddressOf()));
}

void GDxRenderer::CreateRtvAndDsvDescriptorHeaps()
{
	// Add +1 for screen normal map, +2 for ambient maps.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount + 3;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	// Add +1 DSV for shadow map.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 2;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

#pragma endregion

#pragma region Draw

void GDxRenderer::DrawSceneObjects(ID3D12GraphicsCommandList* cmdList, const RenderLayer layer, bool bSetCBV)
{
	// For each render item...
	for (size_t i = 0; i < pSceneObjectLayer[((int)layer)].size(); ++i)
	{
		auto sObject = pSceneObjectLayer[((int)layer)][i];
		DrawSceneObject(cmdList, sObject, bSetCBV);
	}
}

void GDxRenderer::DrawSceneObject(ID3D12GraphicsCommandList* cmdList, GRiSceneObject* sObject, bool bSetCBV)
{
	GDxSceneObject* dxSO = dynamic_cast<GDxSceneObject*>(sObject);
	if (dxSO == NULL)
	{
		ThrowGGiException("Cast failed : from GRiSceneObject* to GDxSceneObject*.")
	}

	GDxMesh* dxMesh = dynamic_cast<GDxMesh*>(sObject->GetMesh());
	if (dxMesh == NULL)
	{
		ThrowGGiException("Cast failed : from GRiMesh* to GDxMesh*.")
	}

	cmdList->IASetVertexBuffers(0, 1, &dxMesh->mVIBuffer->VertexBufferView());
	cmdList->IASetIndexBuffer(&dxMesh->mVIBuffer->IndexBufferView());
	cmdList->IASetPrimitiveTopology(dxSO->GetPrimitiveTopology());

	if (bSetCBV)
	{
		UINT objCBByteSize = GDxUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
		auto objectCB = mCurrFrameResource->ObjectCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + sObject->GetObjIndex() * objCBByteSize;
		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
	}

	cmdList->DrawIndexedInstanced(dxMesh->mVIBuffer->IndexCount, 1, 0, 0, 0);
}

#pragma endregion

#pragma region Runtime

void GDxRenderer::RegisterTexture(GRiTexture* text)
{
	GDxTexture* dxTex = dynamic_cast<GDxTexture*>(text);
	if (dxTex == nullptr)
		ThrowDxException(L"Dynamic cast from GRiTexture to GDxTexture failed.");

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = dxTex->Resource->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = dxTex->Resource->GetDesc().MipLevels;

	// if srv is previously created
	if (dxTex->texIndex != -1)
	{
		md3dDevice->CreateShaderResourceView(dxTex->Resource.Get(), &srvDesc, GetCpuSrv(mTextrueHeapIndex + dxTex->texIndex));
	}
	else
	{
		if (mTexturePoolFreeIndex.empty())
			ThrowGGiException("Texture pool has run out.");
		auto it = mTexturePoolFreeIndex.begin();
		dxTex->texIndex = *it;
		md3dDevice->CreateShaderResourceView(dxTex->Resource.Get(), &srvDesc, GetCpuSrv(mTextrueHeapIndex + *it));
		mTexturePoolFreeIndex.erase(it);
	}
}

GRiSceneObject* GDxRenderer::SelectSceneObject(int sx, int sy)
{
	GGiFloat4x4* P = pCamera->GetProj();

	// Compute picking ray in view space.
	float vx = (+2.0f*sx / mClientWidth - 1.0f) / P->GetElement(0, 0);
	float vy = (-2.0f*sy / mClientHeight + 1.0f) / P->GetElement(1, 1);

	// Ray definition in view space.
	XMVECTOR viewRayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR viewRayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);

	GGiFloat4x4* V = pCamera->GetView();
	GDxFloat4x4* dxV = dynamic_cast<GDxFloat4x4*>(V);
	if (dxV == nullptr)
	{
		ThrowGGiException("cast fail.");
	}
	XMMATRIX dxView = XMLoadFloat4x4(&dxV->GetValue());
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(dxView), dxView);

	GRiSceneObject* pickedSceneObject = nullptr;
	float tPicked = GGiEngineUtil::Infinity;

	// Check if we picked an opaque render item.  A real app might keep a separate "picking list"
	// of objects that can be selected.   
	for (auto so : pSceneObjectLayer[(int)RenderLayer::Deferred])
	{
		auto mesh = so->GetMesh();

		GGiFloat4x4* soTrans = so->GetTransform();
		GDxFloat4x4* dxSoTrans = dynamic_cast<GDxFloat4x4*>(soTrans);
		if (dxSoTrans == nullptr)
		{
			ThrowGGiException("cast fail.");
		}
		XMMATRIX W = XMLoadFloat4x4(&dxSoTrans->GetValue());
		XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(W), W);

		// Tranform ray to vi space of Mesh.
		XMMATRIX toLocal = XMMatrixMultiply(invView, invWorld);

		XMVECTOR rayOrigin = XMVector3TransformCoord(viewRayOrigin, toLocal);
		XMVECTOR rayDir = XMVector3TransformNormal(viewRayDir, toLocal);

		// Make the ray direction unit length for the intersection tests.
		rayDir = XMVector3Normalize(rayDir);

		// If we hit the bounding box of the Mesh, then we might have picked a Mesh triangle,
		// so do the ray/triangle tests.
		//
		// If we did not hit the bounding box, then it is impossible that we hit 
		// the Mesh, so do not waste effort doing ray/triangle tests.
		BoundingBox bBox;
		bBox.Center.x = /*so->GetLocation()[0] +*/ so->GetMesh()->bounds.Center[0];
		bBox.Center.y = /*so->GetLocation()[1] +*/ so->GetMesh()->bounds.Center[1];
		bBox.Center.z = /*so->GetLocation()[2] +*/ so->GetMesh()->bounds.Center[2];
		bBox.Extents.x = so->GetMesh()->bounds.Extents[0];
		bBox.Extents.y = so->GetMesh()->bounds.Extents[1];
		bBox.Extents.z = so->GetMesh()->bounds.Extents[2];
		float tmin = 0.0f;
		if (bBox.Intersects(rayOrigin, rayDir, tmin))
		{
			// NOTE: For the demo, we know what to cast the vertex/index data to.  If we were mixing
			// formats, some metadata would be needed to figure out what to cast it to.
			GDxMesh* dxMesh = dynamic_cast<GDxMesh*>(so->GetMesh());
			if (dxMesh == nullptr)
				ThrowGGiException("cast failed from GRiMesh* to GDxMesh*.");
			shared_ptr<GDxStaticVIBuffer> dxViBuffer = dynamic_pointer_cast<GDxStaticVIBuffer>(dxMesh->mVIBuffer);
			if (dxViBuffer == nullptr)
				ThrowGGiException("cast failed from shared_ptr<GDxStaticVIBuffer> to shared_ptr<GDxStaticVIBuffer>.");
			
			auto vertices = (GRiVertex*)dxViBuffer->VertexBufferCPU->GetBufferPointer();
			auto indices = (std::uint32_t*)dxViBuffer->IndexBufferCPU->GetBufferPointer();
			UINT triCount = dxMesh->mVIBuffer->IndexCount / 3;

			// Find the nearest ray/triangle intersection.
			tmin = GGiEngineUtil::Infinity;
			for (UINT i = 0; i < triCount; ++i)
			{
				// Indices for this triangle.
				UINT i0 = indices[i * 3 + 0];
				UINT i1 = indices[i * 3 + 1];
				UINT i2 = indices[i * 3 + 2];

				// Vertices for this triangle.
				XMFLOAT3 v0f;
				XMFLOAT3 v1f;
				XMFLOAT3 v2f;
				v0f.x = vertices[i0].Position[0];
				v0f.y = vertices[i0].Position[1];
				v0f.z = vertices[i0].Position[2];
				v1f.x = vertices[i1].Position[0];
				v1f.y = vertices[i1].Position[1];
				v1f.z = vertices[i1].Position[2];
				v2f.x = vertices[i2].Position[0];
				v2f.y = vertices[i2].Position[1];
				v2f.z = vertices[i2].Position[2];
				XMVECTOR v0 = XMLoadFloat3(&v0f);
				XMVECTOR v1 = XMLoadFloat3(&v1f);
				XMVECTOR v2 = XMLoadFloat3(&v2f);

				// We have to iterate over all the triangles in order to find the nearest intersection.
				float t = 0.0f;
				if (TriangleTests::Intersects(rayOrigin, rayDir, v0, v1, v2, t))
				{
					if (t < tmin)
					{
						// This is the new nearest picked triangle.
						tmin = t;
					}
				}
			}
			std::vector<float> soScale = so->GetScale();
			float relSize = (float)pow(soScale[0] * soScale[0] + soScale[1] * soScale[1] + soScale[2] * soScale[2], 0.5);
			tmin *= relSize;
			if (tmin < tPicked)
			{
				tPicked = tmin;
				pickedSceneObject = so;
			}
		}
	}
	return pickedSceneObject;
}

#pragma endregion

#pragma region Util

bool GDxRenderer::IsRunning()
{
	if (md3dDevice)
		return true;
	else
		return false;
}

ID3D12Resource* GDxRenderer::CurrentBackBuffer()const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE GDxRenderer::CurrentBackBufferView()const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE GDxRenderer::DepthStencilView()const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void GDxRenderer::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		adapterList.push_back(adapter);

		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void GDxRenderer::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, mBackBufferFormat);

		ReleaseCom(output);

		++i;
	}
}

void GDxRenderer::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	// Call with nullptr to get list count.
	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		::OutputDebugString(text.c_str());
	}
}

void GDxRenderer::FlushCommandQueue()
{
	// Advance the fence value to mark commands up to this fence point.
	mCurrentFence++;

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// Wait until the GPU has completed commands up to this fence point.
	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence.  
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

CD3DX12_CPU_DESCRIPTOR_HANDLE GDxRenderer::GetCpuSrv(int index)const
{
	auto srv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	srv.Offset(index, mCbvSrvUavDescriptorSize);
	return srv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE GDxRenderer::GetGpuSrv(int index)const
{
	auto srv = CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	srv.Offset(index, mCbvSrvUavDescriptorSize);
	return srv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE GDxRenderer::GetDsv(int index)const
{
	auto dsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
	dsv.Offset(index, mDsvDescriptorSize);
	return dsv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE GDxRenderer::GetRtv(int index)const
{
	auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	rtv.Offset(index, mRtvDescriptorSize);
	return rtv;
}

#pragma endregion
