#include "stdafx.h"
#include "GCore.h"
#include <WindowsX.h>
//***************************************************************************************
// SsaoApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************




/*
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		SsaoApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}
*/

GCore::GCore()
{
	// Estimate the scene bounding sphere manually since we know how the scene was constructed.
	// The grid is the "widest object" with a width of 20 and depth of 30.0f, and centered at
	// the world space origin.  In general, you need to loop over every world space vertex
	// position and compute the bounding sphere.
	mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mSceneBounds.Radius = sqrtf(10.0f*10.0f + 15.0f*15.0f);
}

GCore::~GCore()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

GCore* GCore::GetRenderer()
{
	if (mApp == nullptr)
		mApp = new GCore();
	return (GCore*)mApp;
}

bool GCore::Initialize(HWND OutputWindow, double width, double height)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		if (!GRenderer::Initialize(OutputWindow, width, height))
			return false;

		// Reset the command list to prep for initialization commands.
		ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

		mCamera.SetPosition(0.0f, 2.0f, -5.0f);

		mShadowMap = std::make_unique<ShadowMap>(md3dDevice.Get(),
			2048, 2048);

		mSsao = std::make_unique<Ssao>(
			md3dDevice.Get(),
			mCommandList.Get(),
			mClientWidth, mClientHeight);
		BuildCubemapSampleCameras();
		LoadTextures();
		BuildSsaoRootSignature();
		BuildDescriptorHeaps();
		BuildRootSignature();
		BuildShadersAndInputLayout();
		LoadMeshes();
		//BuildSkullGeometry();
		BuildMaterials();
		BuildSceneObjects();
		BuildFrameResources();
		BuildPSOs();

		CubemapPreIntegration();

		mSsao->SetPSOs(mPSOs["ssao"].Get(), mPSOs["ssaoBlur"].Get());
		// Execute the initialization commands.
		ThrowIfFailed(mCommandList->Close());
		ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		// Wait until initialization is complete.
		FlushCommandQueue();

		SetSceneObjectsCallback();

		return true;
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

void GCore::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	/*
	switch (msg)
	{
	case WM_MOUSEMOVE:
		//mTheta += 0.1;
		//mPhi += 0.1;
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return;
	default:
		return;
	}
	*/
	///*
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.
		// We pause the game when the window is deactivated and unpause it
		// when it becomes active.
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			mTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer.Start();
		}
		return; 0;

		// WM_SIZE is sent when the user resizes the window.
	case WM_SIZE:
		// Save the new client area dimensions.
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (md3dDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{

				// Restoring from minimized state?
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if (mResizing)
				{
					// If user is dragging the resize bars, we do not resize
					// the buffers here because as the user continuously
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is
					// done resizing the window and releases the resize bars, which
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return; 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		mTimer.Stop();
		return; 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		mTimer.Start();
		OnResize();
		return; 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return; 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses
		// a key that does not correspond to any mnemonic or accelerator key.
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return; MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return; 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return; 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return; 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return; 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if ((int)wParam == VK_F2)
			Set4xMsaaState(!m4xMsaaState);

		return; 0;
	}
	//*/
}

void GCore::BuildCubemapSampleCameras()
{
	XMFLOAT3 center(0.0f, 0.0f, 0.0f);
	XMFLOAT3 worldUp(0.0f, 1.0f, 0.0f); 

	// Look along each coordinate axis. 
	XMFLOAT3 targets[6] = { 
		XMFLOAT3(1.0f, 0.0f, 0.0f), // +X 
		XMFLOAT3(-1.0f, 0.0f, 0.0f), // -X 
		XMFLOAT3(0.0f, 1.0f, 0.0f), // +Y 
		XMFLOAT3(0.0f, -1.0f, 0.0f), // -Y 
		XMFLOAT3(0.0f, 0.0f, 1.0f), // +Z 
		XMFLOAT3(0.0f, 0.0f, -1.0f) // -Z 
	}; 

	// Use world up vector (0,1,0) for all directions except +Y/-Y.  In these cases, we 
	// are looking down +Y or -Y, so we need a different "up" vector. 
	XMFLOAT3 ups[6] = { 
		XMFLOAT3(0.0f, 1.0f, 0.0f), // +X 
		XMFLOAT3(0.0f, 1.0f, 0.0f), // -X 
		XMFLOAT3(0.0f, 0.0f, -1.0f), // +Y 
		XMFLOAT3(0.0f, 0.0f, +1.0f), // -Y 
		XMFLOAT3(0.0f, 1.0f, 0.0f),	// +Z 
		XMFLOAT3(0.0f, 1.0f, 0.0f)	// -Z 
	}; 

	for(int i = 0; i < 6; ++i) 
	{ 
		mCubemapSampleCamera[i].LookAt(center, targets[i], ups[i]); 
		mCubemapSampleCamera[i].SetLens(0.5f*XM_PI, 1.0f, 0.1f, 1000.0f);
		mCubemapSampleCamera[i].UpdateViewMatrix();
	}
}

void GCore::CubemapPreIntegration()
{
	for (auto i = 0u; i < (6 * mPrefilterLevels); i++)
	{

		PreIntegrationPassCbs.push_back(std::make_unique<UploadBuffer<SkyPassConstants>>(md3dDevice.Get(), 1, true));
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
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		XMMATRIX world = XMLoadFloat4x4(&e->GetTransform());
		XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
		objConstants.MaterialIndex = e->Mat->MatCBIndex;

		currObjectCB->CopyData(e->ObjCBIndex, objConstants);
	}

	// Load sky pass CB.
	for (auto i = 0u; i < mPrefilterLevels; i++)
	{
		for (auto j = 0u; j < 6u; j++)
		{
			XMMATRIX view = mCubemapSampleCamera[j].GetView();
			XMMATRIX proj = mCubemapSampleCamera[j].GetProj();
			XMMATRIX viewProj = XMMatrixMultiply(view, proj);
			XMStoreFloat4x4(&mSkyPassCB.ViewProj, XMMatrixTranspose(viewProj));
			mSkyPassCB.EyePosW = mCamera.GetPosition3f();
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

void GCore::CreateRtvAndDsvDescriptorHeaps()
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

void GCore::OnResize()
{
	GRenderer::OnResize();

	mCamera.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

	if (mSsao != nullptr)
	{
		mSsao->OnResize(mClientWidth, mClientHeight);

		// Resources changed, so need to rebuild descriptors.
		mSsao->RebuildDescriptors(mDepthStencilBuffer.Get());
	}

	for (auto &rtvHeap : mRtvHeaps)
	{
		if (rtvHeap.second->mRtv.size() != 0 && rtvHeap.second->mRtv[0]->mResource != nullptr)
		{
			rtvHeap.second->OnResize(mClientWidth, mClientHeight);
		}
	}
}

void GCore::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);

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

	mLightRotationAngle += 0.1f*gt.DeltaTime();

	XMMATRIX R = XMMatrixRotationY(mLightRotationAngle);
	for (int i = 0; i < 3; ++i)
	{
		XMVECTOR lightDir = XMLoadFloat3(&mBaseLightDirections[i]);
		lightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&mRotatedLightDirections[i], lightDir);
	}

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialBuffer(gt);
	UpdateShadowTransform(gt);
	UpdateMainPassCB(gt);
	UpdateSkyPassCB(gt);
	UpdateShadowPassCB(gt);
	UpdateSsaoCB(gt);
	UpdateLightCB(gt);
}

void GCore::Draw(const GameTimer& gt)
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

	mCommandList->SetGraphicsRootSignature(mRootSignatures["Forward"].Get());

	//
	// Shadow map pass.
	//

	// Bind all the materials used in this scene.  For structured buffers, we can bypass the heap and 
	// set as a root descriptor.
	auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
	mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	// Bind null SRV for shadow map pass.
	mCommandList->SetGraphicsRootDescriptorTable(3, mNullSrv);

	// Bind all the textures used in this scene.  Observe
	// that we only have to specify the first descriptor in the table.  
	// The root signature knows how many descriptors are expected in the table.
	mCommandList->SetGraphicsRootDescriptorTable(4, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	//DrawSceneToShadowMap();

	//
	// Normal/depth pass.
	//

	//DrawNormalsAndDepth();

	//
	// Compute SSAO.
	// 

	//mCommandList->SetGraphicsRootSignature(mSsaoRootSignature.Get());
	//mSsao->ComputeSsao(mCommandList.Get(), mCurrFrameResource, 3);

	//
	// Main rendering pass.
	//

	mCommandList->SetGraphicsRootSignature(mRootSignatures["Forward"].Get());

	// Rebind state whenever graphics root signature changes.

	// Bind all the materials used in this scene.  For structured buffers, we can bypass the heap and 
	// set as a root descriptor.
	matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
	mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);

	// WE ALREADY WROTE THE DEPTH INFO TO THE DEPTH BUFFER IN DrawNormalsAndDepth,
	// SO DO NOT CLEAR DEPTH.
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	// Bind all the textures used in this scene.  Observe
	// that we only have to specify the first descriptor in the table.  
	// The root signature knows how many descriptors are expected in the table.
	mCommandList->SetGraphicsRootDescriptorTable(4, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	// Bind the sky cube map.  For our demos, we just use one "world" cube map representing the environment
	// from far away, so all objects will use the same cube map and we only need to set it once per-frame.  
	// If we wanted to use "local" cube maps, we would have to change them per-object, or dynamically
	// index into an array of cube maps.

	CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	skyTexDescriptor.Offset(mSkyTexHeapIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);

	bool bDeferredRendering = true;
	if (bDeferredRendering)
	{
		// G-Buffer Pass
		{
			mCommandList->RSSetViewports(1, &(mRtvHeaps["GBuffer"]->mRtv[0]->mViewport));
			mCommandList->RSSetScissorRects(1, &(mRtvHeaps["GBuffer"]->mRtv[0]->mScissorRect));

			mCommandList->SetGraphicsRootSignature(mRootSignatures["GBuffer"].Get());

			mCommandList->SetPipelineState(mPSOs["GBuffer"].Get());

			UINT objCBByteSize = GDX12Util::CalcConstantBufferByteSize(sizeof(ObjectConstants));
			auto objectCB = mCurrFrameResource->ObjectCB->Resource();

			auto passCB = mCurrFrameResource->PassCB->Resource();
			mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

			mCommandList->SetGraphicsRootDescriptorTable(2, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

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
			/*
			for (size_t i = 0; i < mSceneObjectLayer[(int)RenderLayer::Deferred].size(); ++i)
			{
				auto ri = mSceneObjectLayer[(int)RenderLayer::Deferred][i];

				mCommandList->IASetVertexBuffers(0, 1, &ri->Mesh->mVIBuffer->VertexBufferView());
				mCommandList->IASetIndexBuffer(&ri->Mesh->mVIBuffer->IndexBufferView());
				mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);

				D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
				mCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);

				mCommandList->DrawIndexedInstanced(ri->GetSubmesh().IndexCount, 1, ri->GetSubmesh().StartIndexLocation, ri->GetSubmesh().BaseVertexLocation, 0);
			}
			*/

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

			mCommandList->SetGraphicsRootDescriptorTable(2, mCubeRtvs["Irradiance"]->GetSrvGpu());

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
			/*
			for (size_t i = 0; i < mRitemLayer[(int)RenderLayer::ScreenQuad].size(); ++i)
			{
				auto ri = mRitemLayer[(int)RenderLayer::ScreenQuad][i];

				mCommandList->IASetVertexBuffers(0, 1, &ri->Mesh->mVIBuffer->VertexBufferView());
				mCommandList->IASetIndexBuffer(&ri->Mesh->mVIBuffer->IndexBufferView());
				mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);
			
				mCommandList->DrawIndexedInstanced(ri->GetSubmesh().IndexCount, 1, ri->GetSubmesh().StartIndexLocation, ri->GetSubmesh().BaseVertexLocation, 0);
			}
			*/

			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRtvHeaps["LightPass"]->mRtv[0]->mResource.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
		}

		// Ambient Light Pass
		{
			mCommandList->RSSetViewports(1, &(mRtvHeaps["LightPass"]->mRtv[1]->mViewport));
			mCommandList->RSSetScissorRects(1, &(mRtvHeaps["LightPass"]->mRtv[1]->mScissorRect));

			mCommandList->SetGraphicsRootSignature(mRootSignatures["LightPass"].Get());

			mCommandList->SetPipelineState(mPSOs["AmbientLightPass"].Get());

			auto lightCB = mCurrFrameResource->LightCB->Resource();
			mCommandList->SetGraphicsRootConstantBufferView(0, lightCB->GetGPUVirtualAddress());

			mCommandList->SetGraphicsRootDescriptorTable(1, mRtvHeaps["GBuffer"]->GetSrvGpuStart());

			mCommandList->SetGraphicsRootDescriptorTable(2, mCubeRtvs["Irradiance"]->GetSrvGpu());

			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRtvHeaps["LightPass"]->mRtv[1]->mResource.Get(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

			// Clear the back buffer.
			DirectX::XMVECTORF32 clearColor = { mRtvHeaps["LightPass"]->mRtv[1]->mProperties.mClearColor[0],
			mRtvHeaps["LightPass"]->mRtv[1]->mProperties.mClearColor[1],
			mRtvHeaps["LightPass"]->mRtv[1]->mProperties.mClearColor[2],
			mRtvHeaps["LightPass"]->mRtv[1]->mProperties.mClearColor[3]
			};

			// WE ALREADY WROTE THE DEPTH INFO TO THE DEPTH BUFFER IN DrawNormalsAndDepth,
			// SO DO NOT CLEAR DEPTH.
			mCommandList->ClearRenderTargetView(mRtvHeaps["LightPass"]->mRtvHeap.handleCPU(1), clearColor, 0, nullptr);

			// Specify the buffers we are going to render to.
			//mCommandList->OMSetRenderTargets(mRtvHeaps["GBuffer"]->mRtvHeap.HeapDesc.NumDescriptors, &(mRtvHeaps["GBuffer"]->mRtvHeap.hCPUHeapStart), true, &DepthStencilView());
			mCommandList->OMSetRenderTargets(1, &(mRtvHeaps["LightPass"]->mRtvHeap.handleCPU(1)), true, &DepthStencilView());

			// For each render item...
			DrawSceneObjects(mCommandList.Get(), RenderLayer::ScreenQuad, false);
			/*
			for (size_t i = 0; i < mRitemLayer[(int)RenderLayer::ScreenQuad].size(); ++i)
			{
				auto ri = mRitemLayer[(int)RenderLayer::ScreenQuad][i];

				mCommandList->IASetVertexBuffers(0, 1, &ri->Mesh->mVIBuffer->VertexBufferView());
				mCommandList->IASetIndexBuffer(&ri->Mesh->mVIBuffer->IndexBufferView());
				mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);

				mCommandList->DrawIndexedInstanced(ri->GetSubmesh().IndexCount, 1, ri->GetSubmesh().StartIndexLocation, ri->GetSubmesh().BaseVertexLocation, 0);
			}
			*/

			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRtvHeaps["LightPass"]->mRtv[1]->mResource.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
		}

		// Debug Pass
		bool bDrawDebugQuad = true;
		if(bDrawDebugQuad)
		{
			mCommandList->RSSetViewports(1, &mScreenViewport);
			mCommandList->RSSetScissorRects(1, &mScissorRect);

			mCommandList->SetGraphicsRootSignature(mRootSignatures["GBufferDebug"].Get());

			mCommandList->SetPipelineState(mPSOs["GBufferDebug"].Get());

			UINT objCBByteSize = GDX12Util::CalcConstantBufferByteSize(sizeof(ObjectConstants));
			auto objectCB = mCurrFrameResource->ObjectCB->Resource();

			mCommandList->SetGraphicsRootDescriptorTable(1, mRtvHeaps["GBuffer"]->GetSrvGpuStart());

			matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
			mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

			mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

			// For each render item...
			DrawSceneObjects(mCommandList.Get(), RenderLayer::Debug, true);
			/*
			for (size_t i = 0; i < mRitemLayer[(int)RenderLayer::Debug].size(); ++i)
			{
				auto ri = mRitemLayer[(int)RenderLayer::Debug][i];

				mCommandList->IASetVertexBuffers(0, 1, &ri->Mesh->mVIBuffer->VertexBufferView());
				mCommandList->IASetIndexBuffer(&ri->Mesh->mVIBuffer->IndexBufferView());
				mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);

				D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
				mCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);

				mCommandList->DrawIndexedInstanced(ri->GetSubmesh().IndexCount, 1, ri->GetSubmesh().StartIndexLocation, ri->GetSubmesh().BaseVertexLocation, 0);
			}
			*/
		}

		// Post Process Pass
		{
			mCommandList->RSSetViewports(1, &mScreenViewport);
			mCommandList->RSSetScissorRects(1, &mScissorRect);

			mCommandList->SetGraphicsRootSignature(mRootSignatures["PostProcess"].Get());

			mCommandList->SetPipelineState(mPSOs["PostProcess"].Get());

			mCommandList->SetGraphicsRootDescriptorTable(0, mRtvHeaps["LightPass"]->GetSrvGpuStart());

			// Specify the buffers we are going to render to.
			//mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
			mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

			// For each render item...
			DrawSceneObjects(mCommandList.Get(), RenderLayer::ScreenQuad, false);
			/*
			for (size_t i = 0; i < mRitemLayer[(int)RenderLayer::ScreenQuad].size(); ++i)
			{
				auto ri = mRitemLayer[(int)RenderLayer::ScreenQuad][i];

				mCommandList->IASetVertexBuffers(0, 1, &ri->Mesh->mVIBuffer->VertexBufferView());
				mCommandList->IASetIndexBuffer(&ri->Mesh->mVIBuffer->IndexBufferView());
				mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);

				mCommandList->DrawIndexedInstanced(ri->GetSubmesh().IndexCount, 1, ri->GetSubmesh().StartIndexLocation, ri->GetSubmesh().BaseVertexLocation, 0);
			}
			*/
		}

		// Reset root parameters and rencer target.
		{
			mCommandList->RSSetViewports(1, &mScreenViewport);
			mCommandList->RSSetScissorRects(1, &mScissorRect);

			mCommandList->SetGraphicsRootSignature(mRootSignatures["Sky"].Get());

			// Specify the buffers we are going to render to.
			mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

			auto passCB = mCurrFrameResource->SkyCB->Resource();
			mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

			///*
			CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			skyTexDescriptor.Offset(mSkyTexHeapIndex, mCbvSrvUavDescriptorSize);
			mCommandList->SetGraphicsRootDescriptorTable(2, skyTexDescriptor);
			//*/

			// Irradiance cubemap debug.
			//CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			//skyTexDescriptor.Offset(mIblIndex + 1, mCbvSrvUavDescriptorSize);
			//mCommandList->SetGraphicsRootDescriptorTable(2, GetGpuSrv(mIblIndex + 2));

			mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
		}

	}


	//mCommandList->SetPipelineState(mPSOs["opaque"].Get());
	//DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	//mCommandList->SetPipelineState(mPSOs["debug"].Get());
	//DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Debug]);

	mCommandList->SetPipelineState(mPSOs["Sky"].Get());
	DrawSceneObjects(mCommandList.Get(), RenderLayer::Sky, true);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void GCore::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void GCore::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void GCore::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mCamera.Pitch(dy);
		mCamera.RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void GCore::OnKeyboardInput(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
		mCamera.Walk(150.0f*dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera.Walk(-150.0f*dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera.Strafe(-150.0f*dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera.Strafe(150.0f*dt);

	if (GetAsyncKeyState('E') & 0x8000)
		mCamera.Ascend(150.0f*dt);

	if (GetAsyncKeyState('Q') & 0x8000)
		mCamera.Ascend(-150.0f*dt);

	mCamera.UpdateViewMatrix();
}

void GCore::AnimateMaterials(const GameTimer& gt)
{

}

void GCore::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX renderObjectTrans = XMLoadFloat4x4(&e->GetTransform());
			//auto tempSubTrans = e->GetSubmesh().Transform;
			//XMMATRIX submeshTrans = XMLoadFloat4x4(&tempSubTrans);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);
			//auto world = submeshTrans * renderObjectTrans;
			auto world = renderObjectTrans;
			
			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world)); 
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
			objConstants.MaterialIndex = e->Mat->MatCBIndex;

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void GCore::UpdateLightCB(const GameTimer& gt)
{
	LightConstants lightCB;

	DirectX::XMStoreFloat3(&(lightCB.cameraPosition), mCamera.GetPosition());

	lightCB.dirLight[0].Direction = XMFLOAT3(0.57735f, -0.57735f, -0.57735f);
	lightCB.dirLight[0].DiffuseColor = XMFLOAT4(0.7f, 0.7f, 0.6f, 1.0f);
	lightCB.dirLight[0].AmbientColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	lightCB.dirLight[0].Intensity = 3.0f;

	lightCB.dirLight[1].Direction = XMFLOAT3(-0.57735f, -0.57735f, -0.57735f);
	lightCB.dirLight[1].DiffuseColor = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	lightCB.dirLight[1].AmbientColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	lightCB.dirLight[1].Intensity = 3.0f;

	lightCB.dirLight[2].Direction = XMFLOAT3(0.0f, -0.707f, 0.707f);
	lightCB.dirLight[2].DiffuseColor = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	lightCB.dirLight[2].AmbientColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	lightCB.dirLight[2].Intensity = 3.0f;

	lightCB.pointLight[0].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0);
	lightCB.pointLight[0].Intensity = 100.0f;
	lightCB.pointLight[0].Position = XMFLOAT3(0.0f, -4.0f, 0.0f);
	lightCB.pointLight[0].Range = 100.0f;

	lightCB.dirLightCount = 3;
	lightCB.pointLightCount = 0;

	auto LightCB = mCurrFrameResource->LightCB.get();
	LightCB->CopyData(0, lightCB);
}

void GCore::UpdateMaterialBuffer(const GameTimer& gt)
{
	auto currMaterialBuffer = mCurrFrameResource->MaterialBuffer.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		GMaterial* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{

			MaterialData matData;
			int i;
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));

			if (mat->mTextures.size() > MATERIAL_MAX_TEXTURE_NUM)
				ThrowDxException(L"Material (CBIndex : " + std::to_wstring(mat->MatCBIndex) + L" ) texture number exceeds MATERIAL_MAX_TEXTURE_NUM.");
			i = 0;
			for ( auto tex : mat->mTextures)
			{
				matData.TextureIndex[i] = tex->descriptorHeapIndex;
				i++;
			}
			if (mat->ScalarParams.size() > MATERIAL_MAX_SCALAR_NUM)
				ThrowDxException(L"Material (CBIndex : " + std::to_wstring(mat->MatCBIndex) + L" ) scalar number exceeds MATERIAL_MAX_SCALAR_NUM.");
			i = 0;
			for (auto scalar : mat->ScalarParams)
			{
				matData.ScalarParams[i] = scalar;
				i++;
			}
			if (mat->ScalarParams.size() > MATERIAL_MAX_VECTOR_NUM)
				ThrowDxException(L"Material (CBIndex : " + std::to_wstring(mat->MatCBIndex) + L" ) vector number exceeds MATERIAL_MAX_VECTOR_NUM.");
			i = 0;
			for (auto vector : mat->VectorParams)
			{
				matData.VectorParams[i] = vector;
				i++;
			}
			//matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
			//matData.NormalMapIndex = mat->NormalSrvHeapIndex;
			//matData.Roughness = mat->Roughness;
			//matData.DiffuseAlbedo = mat->DiffuseAlbedo;
			//matData.FresnelR0 = mat->FresnelR0;

			currMaterialBuffer->CopyData(mat->MatCBIndex, matData);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void GCore::UpdateShadowTransform(const GameTimer& gt)
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

void GCore::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

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
	mMainPassCB.EyePosW = mCamera.GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.4f, 0.4f, 0.6f, 1.0f };
	/*
	mMainPassCB.Lights[0].Direction = mRotatedLightDirections[0];
	mMainPassCB.Lights[0].Strength = { 0.7f, 0.7f, 0.6f };
	mMainPassCB.Lights[1].Direction = mRotatedLightDirections[1];
	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[2].Direction = mRotatedLightDirections[2];
	mMainPassCB.Lights[2].Strength = { 0.1f, 0.1f, 0.1f };
	*/

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void GCore::UpdateSkyPassCB(const GameTimer& gt)
{
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMStoreFloat4x4(&mSkyPassCB.ViewProj, XMMatrixTranspose(viewProj));
	mSkyPassCB.EyePosW = mCamera.GetPosition3f();
	mSkyPassCB.roughness = 0.3f; // doesn't matter

	auto currPassCB = mCurrFrameResource->SkyCB.get();
	currPassCB->CopyData(0, mSkyPassCB);
}

void GCore::UpdateShadowPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mLightView);
	XMMATRIX proj = XMLoadFloat4x4(&mLightProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	UINT w = mShadowMap->Width();
	UINT h = mShadowMap->Height();

	XMStoreFloat4x4(&mShadowPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mShadowPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mShadowPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mShadowPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mShadowPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mShadowPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mShadowPassCB.EyePosW = mLightPosW;
	mShadowPassCB.RenderTargetSize = XMFLOAT2((float)w, (float)h);
	mShadowPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / w, 1.0f / h);
	mShadowPassCB.NearZ = mLightNearZ;
	mShadowPassCB.FarZ = mLightFarZ;

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(1, mShadowPassCB);
}

void GCore::UpdateSsaoCB(const GameTimer& gt)
{
	SsaoConstants ssaoCB;

	XMMATRIX P = mCamera.GetProj();

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	ssaoCB.Proj = mMainPassCB.Proj;
	ssaoCB.InvProj = mMainPassCB.InvProj;
	XMStoreFloat4x4(&ssaoCB.ProjTex, XMMatrixTranspose(P*T));

	mSsao->GetOffsetVectors(ssaoCB.OffsetVectors);

	auto blurWeights = mSsao->CalcGaussWeights(2.5f);
	ssaoCB.BlurWeights[0] = XMFLOAT4(&blurWeights[0]);
	ssaoCB.BlurWeights[1] = XMFLOAT4(&blurWeights[4]);
	ssaoCB.BlurWeights[2] = XMFLOAT4(&blurWeights[8]);

	ssaoCB.InvRenderTargetSize = XMFLOAT2(1.0f / mSsao->SsaoMapWidth(), 1.0f / mSsao->SsaoMapHeight());

	// Coordinates given in view space.
	ssaoCB.OcclusionRadius = 0.5f;
	ssaoCB.OcclusionFadeStart = 0.2f;
	ssaoCB.OcclusionFadeEnd = 1.0f;
	ssaoCB.SurfaceEpsilon = 0.05f;

	auto currSsaoCB = mCurrFrameResource->SsaoCB.get();
	currSsaoCB->CopyData(0, ssaoCB);
}

void GCore::LoadTextures()
{
	//
	// Load non-DDS images.
	//
	std::vector<std::string> texNames =
	{
		"IBL_BRDF_LUT",
		"default_albedo",
		"default_normal",
		"default_OcclussionRoughnessMetallic",
		"sphere_1_BaseColor",
		"sphere_1_Normal",
		"sphere_1_OcclusionRoughnessMetallic",
		"sphere_2_BaseColor",
		"sphere_2_Normal",
		"sphere_2_OcclusionRoughnessMetallic",
		"Greasy_Pan_Albedo",
		"Greasy_Pan_Normal",
		"Greasy_Pan_Orm",
		"Rusted_Iron_Albedo",
		"Rusted_Iron_Normal",
		"Rusted_Iron_Orm",
		"Cerberus_Albedo",
		"Cerberus_Normal",
		"Cerberus_Orm",
		"Fireplace_Albedo",
		"Fireplace_Normal",
		"Fireplace_Orm"
		/*
		,"bricksDiffuseMap",
		"bricksNormalMap",
		"tileDiffuseMap",
		"tileNormalMap",
		"defaultDiffuseMap",
		"defaultNormalMap",
		"skyCubeMap"
		*/
	};

	std::vector<std::wstring> texFilenames =
	{
		L"Textures/IBL_BRDF_LUT.png",
		L"Textures/default_albedo.png",
		L"Textures/default_normal.png",
		L"Textures/default_OcclussionRoughnessMetallic.png",
		L"Textures/sphere_1_BaseColor.png",
		L"Textures/sphere_1_Normal.png",
		L"Textures/sphere_1_OcclusionRoughnessMetallic.png",
		L"Textures/sphere_2_BaseColor.png",
		L"Textures/sphere_2_Normal.png",
		L"Textures/sphere_2_OcclusionRoughnessMetallic.png",
		L"Textures/Greasy_Pan_Albedo.png",
		L"Textures/Greasy_Pan_Normal.png",
		L"Textures/Greasy_Pan_Orm.png",
		L"Textures/Rusted_Iron_Albedo.png",
		L"Textures/Rusted_Iron_Normal.png",
		L"Textures/Rusted_Iron_Orm.png",
		L"Textures/Cerberus_Albedo.png",
		L"Textures/Cerberus_Normal.png",
		L"Textures/Cerberus_Orm.png",
		L"Textures/Fireplace_Albedo.png",
		L"Textures/Fireplace_Normal.png",
		L"Textures/Fireplace_Orm.png"
		/*
		,L"Textures/bricks2.tga",
		L"Textures/bricks2_nmap.tga",
		L"Textures/tile.tga",
		L"Textures/tile_nmap.tga",
		L"Textures/white1x1.tga",
		L"Textures/default_nmap.tga",
		L"Textures/sunsetcube1024.dds"
		*/
	};

	std::vector<bool> texSrgb =
	{
		false,
		true,
		false,
		false,
		true,
		false,
		false,
		true,
		false,
		false,
		true,
		false,
		false,
		true,
		false,
		false,
		true,
		false,
		false,
		true,
		false,
		false
	};

	for (int i = 0; i < (int)texNames.size(); ++i)
	{
		/*
		auto texMap = std::make_shared<GTexture>();
		texMap->Name = texNames[i];
		texMap->Filename = texFilenames[i];
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
			mCommandList.Get(), texMap->Filename.c_str(),
			texMap->Resource, texMap->UploadHeap));

		mTextures[texMap->Name] = std::move(texMap);
		*/
		///*
		auto texMap = std::make_shared<GTexture>();
		texMap->Name = texNames[i];
		texMap->Filename = texFilenames[i];
		texMap->descriptorHeapIndex = i;
		ResourceUploadBatch resourceUpload(md3dDevice.Get());
		resourceUpload.Begin();
		unsigned int srgbFlag;
		if (texSrgb[i])
		{
			srgbFlag = WIC_LOADER_FORCE_SRGB;
		}
		else
		{
			srgbFlag = WIC_LOADER_IGNORE_SRGB;
		}
		//ThrowIfFailed(CreateWICTextureFromFile(md3dDevice.Get(), resourceUpload, texMap->Filename.c_str(), texMap->Resource.ReleaseAndGetAddressOf()));
		ThrowIfFailed(CreateWICTextureFromFileEx(md3dDevice.Get(), resourceUpload, texMap->Filename.c_str(), 0u, D3D12_RESOURCE_FLAG_NONE, srgbFlag, texMap->Resource.ReleaseAndGetAddressOf()));
		auto uploadResourcesFinished = resourceUpload.End(mCommandQueue.Get());
		uploadResourcesFinished.wait();

		//CreateShaderResourceView(md3dDevice.Get(), texMap->Resource.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Cat));
		std::shared_ptr<GTexture> texListPtr(texMap);
		mTextureList.push_back(texListPtr);
		mTextures[texMap->Name] = std::move(texMap);
		//*/
	}

	//
	// Load DDS images.
	//
	std::vector<std::string> ddsTexNames =
	{
		"bricksDiffuseMap",
		"bricksNormalMap",
		"tileDiffuseMap",
		"tileNormalMap",
		"defaultDiffuseMap",
		"defaultNormalMap",
		"skyCubeMap"
	};

	std::vector<std::wstring> ddsTexFilenames =
	{
		L"Textures/bricks2.dds",
		L"Textures/bricks2_nmap.dds",
		L"Textures/tile.dds",
		L"Textures/tile_nmap.dds",
		L"Textures/white1x1.dds",
		L"Textures/default_nmap.dds",
		//L"Textures/sunsetcube1024.dds"
		L"Textures/Cubemap_LancellottiChapel.dds"
	};

	for (int i = 0; i < (int)ddsTexNames.size(); ++i)
	{
		auto texMap = std::make_shared<GTexture>();
		texMap->Name = ddsTexNames[i];
		texMap->Filename = ddsTexFilenames[i];
		texMap->descriptorHeapIndex = i + (int)texNames.size();
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
			mCommandList.Get(), texMap->Filename.c_str(),
			texMap->Resource, texMap->UploadHeap));

		std::shared_ptr<GTexture> texListPtr(texMap);
		mTextureList.push_back(texListPtr);
		mTextures[texMap->Name] = std::move(texMap);
	}
}

void GCore::BuildRootSignature()
{
	// GBuffer root signature
	{

	//G-Buffer inputs
	CD3DX12_DESCRIPTOR_RANGE range;
	range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)mTextureList.size(), 0);

	CD3DX12_ROOT_PARAMETER gBufferRootParameters[4];
	//gBufferRootParameters[0].InitAsDescriptorTable(1, &range[0], D3D12_SHADER_VISIBILITY_VERTEX);
	//gBufferRootParameters[1].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_PIXEL);
	gBufferRootParameters[0].InitAsConstantBufferView(0);
	gBufferRootParameters[1].InitAsConstantBufferView(1);
	//gBufferRootParameters[2].InitAsShaderResourceView(0, 0);//albedo
	//gBufferRootParameters[3].InitAsShaderResourceView(1, 0);//normal
	//gBufferRootParameters[4].InitAsShaderResourceView(2, 0);//occlusion roughness metallic
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
		rangeIBL.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)mCubeRtvs.size() + (UINT)1, mRtvHeaps["GBuffer"]->mRtvHeap.HeapDesc.NumDescriptors);

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
		CD3DX12_DESCRIPTOR_RANGE range;
		range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, mRtvHeaps["LightPass"]->mRtvHeap.HeapDesc.NumDescriptors, 0);

		CD3DX12_ROOT_PARAMETER gPostProcessRootParameters[1];
		gPostProcessRootParameters[0].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_ALL);

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, gPostProcessRootParameters,
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
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)mTextureList.size(), 3, 0);//10,3,0

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

void GCore::BuildSsaoRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstants(1, 1);
	slotRootParameter[2].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC depthMapSam(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,
		0,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	std::array<CD3DX12_STATIC_SAMPLER_DESC, 4> staticSamplers =
	{
		pointClamp, linearClamp, depthMapSam, linearWrap
	};

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
		IID_PPV_ARGS(mSsaoRootSignature.GetAddressOf())));
}

void GCore::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = (UINT)mTextures.size() + mPrefilterLevels + 18;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	
	/*
	std::vector<ComPtr<ID3D12Resource>> tex2DList =
	{
		mTextures["bricksDiffuseMap"]->Resource,
		mTextures["bricksNormalMap"]->Resource,
		mTextures["tileDiffuseMap"]->Resource,
		mTextures["tileNormalMap"]->Resource,
		mTextures["defaultDiffuseMap"]->Resource,
		mTextures["defaultNormalMap"]->Resource
	};
	*/

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;


	for (UINT i = 0; i < (UINT)mTextureList.size(); ++i)
	{
		if (mTextureList[i]->Name == "skyCubeMap")
			continue;
		srvDesc.Format = mTextureList[i]->Resource->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = mTextureList[i]->Resource->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(mTextureList[i]->Resource.Get(), &srvDesc, hDescriptor);

		// next descriptor
		hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}

	/*
	auto skyCubeMap = mLegTextures["skyCubeMap"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	srvDesc.Format = tex2DList[i]->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = tex2DList[i]->GetDesc().MipLevels;

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = skyCubeMap->GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = skyCubeMap->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(skyCubeMap.Get(), &srvDesc, hDescriptor);
	*/

	//auto skyCubeMap = mTextures["skyCubeMap"]->Resource;
	D3D12_SHADER_RESOURCE_VIEW_DESC skySrvDesc = {};
	skySrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	skySrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	skySrvDesc.TextureCube.MostDetailedMip = 0;
	skySrvDesc.TextureCube.MipLevels = mTextures["skyCubeMap"]->Resource->GetDesc().MipLevels;
	skySrvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	skySrvDesc.Format = mTextures["skyCubeMap"]->Resource->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(mTextures["skyCubeMap"]->Resource.Get(), &skySrvDesc, hDescriptor);

	mSkyTexHeapIndex = (UINT)mTextureList.size() - 1;
	mShadowMapHeapIndex = mSkyTexHeapIndex + 1;
	mSsaoHeapIndexStart = mShadowMapHeapIndex + 1;
	mSsaoAmbientMapIndex = mSsaoHeapIndexStart + 3;
	mNullCubeSrvIndex = mSsaoHeapIndexStart + 5;
	mNullTexSrvIndex1 = mNullCubeSrvIndex + 1;
	mNullTexSrvIndex2 = mNullTexSrvIndex1 + 1;

	auto nullSrv = GetCpuSrv(mNullCubeSrvIndex);
	mNullSrv = GetGpuSrv(mNullCubeSrvIndex);

	md3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);
	nullSrv.Offset(1, mCbvSrvUavDescriptorSize);

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	md3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

	nullSrv.Offset(1, mCbvSrvUavDescriptorSize);
	md3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

	mShadowMap->BuildDescriptors(
		GetCpuSrv(mShadowMapHeapIndex),
		GetGpuSrv(mShadowMapHeapIndex),
		GetDsv(1));

	mSsao->BuildDescriptors(
		mDepthStencilBuffer.Get(),
		GetCpuSrv(mSsaoHeapIndexStart),
		GetGpuSrv(mSsaoHeapIndexStart),
		GetRtv(SwapChainBufferCount),
		mCbvSrvUavDescriptorSize,
		mRtvDescriptorSize);

	// Build RTV heap and SRV for GBuffers.
	{
		mGBufferSrvIndex = mNullCubeSrvIndex + 1;

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
		auto gBufferRtvHeap = std::make_unique<GRtvHeap>(md3dDevice.Get(), mClientWidth, mClientHeight, GetCpuSrv(mGBufferSrvIndex), GetGpuSrv(mGBufferSrvIndex), propVec);
		mRtvHeaps["GBuffer"] = std::move(gBufferRtvHeap);
	}

	// Build RTV heap and SRV for light pass.
	{
		mLightPassSrvIndex = mGBufferSrvIndex + mRtvHeaps["GBuffer"]->mRtvHeap.HeapDesc.NumDescriptors;

		std::vector<DXGI_FORMAT> rtvFormats =
		{
			DXGI_FORMAT_R32G32B32A32_FLOAT,// Direct light
			DXGI_FORMAT_R32G32B32A32_FLOAT// Ambient light
		};
		std::vector<std::vector<FLOAT>> rtvClearColor =
		{
			{ 0,0,0,0 },
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
		auto gBufferRtvHeap = std::make_unique<GRtvHeap>(md3dDevice.Get(), mClientWidth, mClientHeight, GetCpuSrv(mLightPassSrvIndex), GetGpuSrv(mLightPassSrvIndex), propVec);
		mRtvHeaps["LightPass"] = std::move(gBufferRtvHeap);
	}

	// Build cubemap SRV and RTVs for irradiance pre-integration.
	{
		mIblIndex = mLightPassSrvIndex + 2;

		GRtvProperties prop;
		prop.mRtvFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		prop.mClearColor[0] = 0;
		prop.mClearColor[1] = 0;
		prop.mClearColor[2] = 0;
		prop.mClearColor[3] = 1;

		auto gIrradianceCubemap = std::make_unique<GCubeRtv>(md3dDevice.Get(), 2048, GetCpuSrv(mIblIndex), GetGpuSrv(mIblIndex), prop); 
		mCubeRtvs["Irradiance"] = std::move(gIrradianceCubemap);
	}

	// Build SRV for LUT
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		srvDesc.Format = mTextures["IBL_BRDF_LUT"]->Resource->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = mTextures["IBL_BRDF_LUT"]->Resource->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(mTextures["IBL_BRDF_LUT"]->Resource.Get(), &srvDesc, GetCpuSrv(mIblIndex + 1));
	}

	// Build cubemap SRV and RTVs for prefilter cubemap pre-integration.
	for (auto i = 0u; i < mPrefilterLevels; i++)
	{
		GRtvProperties prop;
		prop.mRtvFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		prop.mClearColor[0] = 0;
		prop.mClearColor[1] = 0;
		prop.mClearColor[2] = 0;
		prop.mClearColor[3] = 1;

		auto gPrefilterCubemap = std::make_unique<GCubeRtv>(md3dDevice.Get(), (UINT)(2048 / pow(2, i)), GetCpuSrv(mIblIndex + 2 + i), GetGpuSrv(mIblIndex + 2 + i), prop);
		mCubeRtvs["Prefilter_" + std::to_string(i)] = std::move(gPrefilterCubemap);
	}
}

void GCore::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	/*
	mShaders["standardVS"] = GShaderManager::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = GShaderManager::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["shadowVS"] = GShaderManager::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["shadowOpaquePS"] = GShaderManager::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["shadowAlphaTestedPS"] = GShaderManager::CompileShader(L"Shaders\\Shadows.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mShaders["debugVS"] = GShaderManager::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["debugPS"] = GShaderManager::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["drawNormalsVS"] = GShaderManager::CompileShader(L"Shaders\\DrawNormals.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["drawNormalsPS"] = GShaderManager::CompileShader(L"Shaders\\DrawNormals.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["ssaoVS"] = GShaderManager::CompileShader(L"Shaders\\Ssao.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ssaoPS"] = GShaderManager::CompileShader(L"Shaders\\Ssao.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["ssaoBlurVS"] = GShaderManager::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ssaoBlurPS"] = GShaderManager::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["skyVS"] = GShaderManager::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["skyPS"] = GShaderManager::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");
	*/

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void GCore::LoadMeshes()
{
	GeometryGenerator geoGen;

	std::vector<MeshData> meshData;
	MeshData boxMeshData = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
	boxMeshData.SubmeshName = "Box";
	meshData.push_back(boxMeshData);
	auto geo = std::make_shared<GMesh>(md3dDevice.Get(), mCommandList.Get(), meshData);
	geo->Name = "Box";
	mMeshes[geo->Name] = std::move(geo);

	meshData.clear();
	MeshData gridMeshData = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
	gridMeshData.SubmeshName = "Grid";
	meshData.push_back(gridMeshData);
	geo = std::make_shared<GMesh>(md3dDevice.Get(), mCommandList.Get(), meshData);
	geo->Name = "Grid";
	mMeshes[geo->Name] = std::move(geo);

	meshData.clear();
	MeshData sphereMeshData = geoGen.CreateSphere(0.5f, 20, 20);
	sphereMeshData.SubmeshName = "Sphere";
	meshData.push_back(sphereMeshData);
	geo = std::make_shared<GMesh>(md3dDevice.Get(), mCommandList.Get(), meshData);
	geo->Name = "Sphere";
	mMeshes[geo->Name] = std::move(geo);

	meshData.clear();
	MeshData cylinderMeshData = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
	cylinderMeshData.SubmeshName = "Cylinder";
	meshData.push_back(cylinderMeshData);
	geo = std::make_shared<GMesh>(md3dDevice.Get(), mCommandList.Get(), meshData);
	geo->Name = "Cylinder";
	mMeshes[geo->Name] = std::move(geo);

	meshData.clear();
	MeshData quadMeshData = geoGen.CreateQuad(0.0f, 1.0f, 1.0f, 1.0f, 0.0f);
	quadMeshData.SubmeshName = "Quad";
	meshData.push_back(quadMeshData);
	geo = std::make_shared<GMesh>(md3dDevice.Get(), mCommandList.Get(), meshData);
	geo->Name = "Quad";
	mMeshes[geo->Name] = std::move(geo);

	GMesh* CerberusMesh = new GMesh();
	GFilmboxManager::GetManager().ImportFbxFile_Mesh(md3dDevice.Get(), mCommandList.Get(), "Models\\Cerberus.FBX", CerberusMesh);
	CerberusMesh->Name = "Cerberus";
	std::shared_ptr<GMesh> cerberus(CerberusMesh);
	mMeshes[cerberus->Name] = cerberus;

	GMesh* FireplaceMesh = new GMesh();
	GFilmboxManager::GetManager().ImportFbxFile_Mesh(md3dDevice.Get(), mCommandList.Get(), "Models\\Fireplace.FBX", FireplaceMesh);
	FireplaceMesh->Name = "Fireplace";
	std::shared_ptr<GMesh> fireplace(FireplaceMesh);
	mMeshes[fireplace->Name] = fireplace;
}

/*
void GCore::BuildSkullGeometry()
{
	std::ifstream fin("Models/skull.txt");

	if (!fin)
	{
		MessageBox(0, L"Models/skull.txt not found.", 0, 0);
		return;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
	XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

	XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

	std::vector<Vertex> vertices(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

		vertices[i].TexC = { 0.0f, 0.0f };

		XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

		XMVECTOR N = XMLoadFloat3(&vertices[i].Normal);

		// Generate a tangent vector so normal mapping works.  We aren't applying
		// a texture map to the skull, so we just need any tangent vector so that
		// the math works out to give us the original interpolated vertex normal.
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		if (fabsf(XMVectorGetX(XMVector3Dot(N, up))) < 1.0f - 0.001f)
		{
			XMVECTOR T = XMVector3Normalize(XMVector3Cross(up, N));
			XMStoreFloat3(&vertices[i].TangentU, T);
		}
		else
		{
			up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
			XMVECTOR T = XMVector3Normalize(XMVector3Cross(N, up));
			XMStoreFloat3(&vertices[i].TangentU, T);
		}


		vMin = XMVectorMin(vMin, P);
		vMax = XMVectorMax(vMax, P);
	}

	BoundingBox bounds;
	XMStoreFloat3(&bounds.Center, 0.5f*(vMin + vMax));
	XMStoreFloat3(&bounds.Extents, 0.5f*(vMax - vMin));

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	std::vector<std::int32_t> indices(3 * tcount);
	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::int32_t);

	auto geo = std::make_unique<GMeshGeometry>();
	geo->Name = "skullGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = GDX12Util::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = GDX12Util::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	GSubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;
	submesh.Bounds = bounds;

	geo->DrawArgs["skull"] = submesh;

	mGeometries[geo->Name] = std::move(geo);
}
*/

void GCore::BuildPSOs()
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
	gBufferPsoDesc.VS = GShaderManager::LoadShader(L"Shaders\\DefaultVS.cso");
	gBufferPsoDesc.PS = GShaderManager::LoadShader(L"Shaders\\DeferredPS.cso");
	gBufferPsoDesc.InputLayout.pInputElementDescs = GInputLayout::DefaultLayout;
	gBufferPsoDesc.InputLayout.NumElements = _countof(GInputLayout::DefaultLayout);
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

	/*
	descPipelineState.VS = GShaderManager::LoadShader(L"AnimationDefaultVS.cso");
	descPipelineState.InputLayout.pInputElementDescs = InputLayout::AnimationLayout;
	descPipelineState.InputLayout.NumElements = _countof(InputLayout::AnimationLayout);
	auto animDeferredPSO = sysRM->CreatePSO(StringID("animDeferredPSO"), descPipelineState);

	descPipelineState.VS = GShaderManager::LoadShader(L"InstancedDefaultVS.cso");
	descPipelineState.InputLayout.pInputElementDescs = InputLayout::InstanceDefaultLayout;
	descPipelineState.InputLayout.NumElements = _countof(InputLayout::InstanceDefaultLayout);
	sysRM->CreatePSO(StringID("instancedDeferredPSO"), descPipelineState);
	*/
	
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
	lightPassDSD.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	// We are not rendering backfacing polygons, so these settings do not matter. 
	lightPassDSD.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	lightPassDSD.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	lightPassDSD.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	lightPassDSD.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

	auto blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	blendState.AlphaToCoverageEnable = false;
	blendState.IndependentBlendEnable = false;

	blendState.RenderTarget[0].BlendEnable = true;
	blendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

	auto rasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//rasterizer.CullMode = D3D12_CULL_MODE_FRONT; // Front culling for point light
	rasterizer.CullMode = D3D12_CULL_MODE_NONE;
	rasterizer.DepthClipEnable = false;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState;
	ZeroMemory(&descPipelineState, sizeof(descPipelineState));

	descPipelineState.VS = GShaderManager::LoadShader(L"Shaders\\FullScreenVS.cso");
	descPipelineState.PS = GShaderManager::LoadShader(L"Shaders\\DirectLightPassPS.cso");
	descPipelineState.pRootSignature = mRootSignatures["LightPass"].Get();
	descPipelineState.BlendState = blendState;
	descPipelineState.DepthStencilState = lightPassDSD;
	descPipelineState.DepthStencilState.DepthEnable = false;
	descPipelineState.InputLayout.pInputElementDescs = GInputLayout::DefaultLayout;
	descPipelineState.InputLayout.NumElements = _countof(GInputLayout::DefaultLayout);
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
	ambientPassDSD.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	// We are not rendering backfacing polygons, so these settings do not matter. 
	ambientPassDSD.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	ambientPassDSD.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	ambientPassDSD.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	ambientPassDSD.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

	auto ambientBlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	ambientBlendState.AlphaToCoverageEnable = false;
	ambientBlendState.IndependentBlendEnable = false;

	ambientBlendState.RenderTarget[0].BlendEnable = true;
	ambientBlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	ambientBlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	ambientBlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

	auto ambientRasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//rasterizer.CullMode = D3D12_CULL_MODE_FRONT; // Front culling for point light
	ambientRasterizer.CullMode = D3D12_CULL_MODE_NONE;
	ambientRasterizer.DepthClipEnable = false;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC descAmbientPSO;
	ZeroMemory(&descAmbientPSO, sizeof(descAmbientPSO));

	descAmbientPSO.VS = GShaderManager::LoadShader(L"Shaders\\FullScreenVS.cso");
	descAmbientPSO.PS = GShaderManager::LoadShader(L"Shaders\\AmbientPassPS.cso");
	descAmbientPSO.pRootSignature = mRootSignatures["LightPass"].Get();
	descAmbientPSO.BlendState = ambientBlendState;
	descAmbientPSO.DepthStencilState = ambientPassDSD;
	descAmbientPSO.DepthStencilState.DepthEnable = false;
	descAmbientPSO.InputLayout.pInputElementDescs = GInputLayout::DefaultLayout;
	descAmbientPSO.InputLayout.NumElements = _countof(GInputLayout::DefaultLayout);
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
	postProcessDSD.StencilEnable = true;
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
	PostProcessPsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	PostProcessPsoDesc.pRootSignature = mRootSignatures["PostProcess"].Get();
	PostProcessPsoDesc.VS = GShaderManager::LoadShader(L"Shaders\\FullScreenVS.cso");
	PostProcessPsoDesc.PS = GShaderManager::LoadShader(L"Shaders\\PostProcessPS.cso");
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
	D3D12_GRAPHICS_PIPELINE_STATE_DESC basePsoDesc;


	ZeroMemory(&basePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	basePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	basePsoDesc.pRootSignature = mRootSignatures["Forward"].Get();
	/*
	basePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	basePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	*/
	basePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	basePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	basePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	basePsoDesc.SampleMask = UINT_MAX;
	basePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	basePsoDesc.NumRenderTargets = 1;
	basePsoDesc.RTVFormats[0] = mBackBufferFormat;
	basePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	basePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	basePsoDesc.DSVFormat = mDepthStencilFormat;

	//
	// PSO for opaque objects.
	//
	/*
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc = basePsoDesc;
	opaquePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
	opaquePsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
	*/

	//
	// PSO for shadow map pass.
	//
	/*
	D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = basePsoDesc;
	smapPsoDesc.RasterizerState.DepthBias = 100000;
	smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	smapPsoDesc.pRootSignature = mRootSignatures["Forward"].Get();
	smapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["shadowVS"]->GetBufferPointer()),
		mShaders["shadowVS"]->GetBufferSize()
	};
	smapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["shadowOpaquePS"]->GetBufferPointer()),
		mShaders["shadowOpaquePS"]->GetBufferSize()
	};

	// Shadow map pass does not have a render target.
	smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	smapPsoDesc.NumRenderTargets = 0;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&mPSOs["shadow_opaque"])));
	*/
	//
	// PSO for debug layer.
	//
	/*
	D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPsoDesc = basePsoDesc;
	debugPsoDesc.pRootSignature = mRootSignatures["Forward"].Get();
	debugPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["debugVS"]->GetBufferPointer()),
		mShaders["debugVS"]->GetBufferSize()
	};
	debugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["debugPS"]->GetBufferPointer()),
		mShaders["debugPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&debugPsoDesc, IID_PPV_ARGS(&mPSOs["debug"])));
	*/


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

	D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPsoDesc = basePsoDesc;
	debugPsoDesc.pRootSignature = mRootSignatures["GBufferDebug"].Get();
	debugPsoDesc.VS = GShaderManager::LoadShader(L"Shaders\\ScreenVS.cso");
	debugPsoDesc.PS = GShaderManager::LoadShader(L"Shaders\\GBufferDebugPS.cso");
	debugPsoDesc.DepthStencilState = gBufferDebugDSD;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&debugPsoDesc, IID_PPV_ARGS(&mPSOs["GBufferDebug"])));

	//
	// PSO for drawing normals.
	//
	/*
	D3D12_GRAPHICS_PIPELINE_STATE_DESC drawNormalsPsoDesc = basePsoDesc;
	drawNormalsPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["drawNormalsVS"]->GetBufferPointer()),
		mShaders["drawNormalsVS"]->GetBufferSize()
	};
	drawNormalsPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["drawNormalsPS"]->GetBufferPointer()),
		mShaders["drawNormalsPS"]->GetBufferSize()
	};
	drawNormalsPsoDesc.RTVFormats[0] = Ssao::NormalMapFormat;
	drawNormalsPsoDesc.SampleDesc.Count = 1;
	drawNormalsPsoDesc.SampleDesc.Quality = 0;
	drawNormalsPsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&drawNormalsPsoDesc, IID_PPV_ARGS(&mPSOs["drawNormals"])));
	*/

	//
	// PSO for SSAO.
	//
	/*
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoPsoDesc = basePsoDesc;
	ssaoPsoDesc.InputLayout = { nullptr, 0 };
	ssaoPsoDesc.pRootSignature = mSsaoRootSignature.Get();
	ssaoPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["ssaoVS"]->GetBufferPointer()),
		mShaders["ssaoVS"]->GetBufferSize()
	};
	ssaoPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["ssaoPS"]->GetBufferPointer()),
		mShaders["ssaoPS"]->GetBufferSize()
	};

	// SSAO effect does not need the depth buffer.
	ssaoPsoDesc.DepthStencilState.DepthEnable = false;
	ssaoPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ssaoPsoDesc.RTVFormats[0] = Ssao::AmbientMapFormat;
	ssaoPsoDesc.SampleDesc.Count = 1;
	ssaoPsoDesc.SampleDesc.Quality = 0;
	ssaoPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&ssaoPsoDesc, IID_PPV_ARGS(&mPSOs["ssao"])));
	*/
	//
	// PSO for SSAO blur.
	//
	/*
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoBlurPsoDesc = ssaoPsoDesc;
	ssaoBlurPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["ssaoBlurVS"]->GetBufferPointer()),
		mShaders["ssaoBlurVS"]->GetBufferSize()
	};
	ssaoBlurPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["ssaoBlurPS"]->GetBufferPointer()),
		mShaders["ssaoBlurPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&ssaoBlurPsoDesc, IID_PPV_ARGS(&mPSOs["ssaoBlur"])));
	*/

	//
	// PSO for sky.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc;

	ZeroMemory(&skyPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	skyPsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	skyPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	skyPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	skyPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	skyPsoDesc.SampleMask = UINT_MAX;
	skyPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	skyPsoDesc.NumRenderTargets = 1;
	skyPsoDesc.RTVFormats[0] = mBackBufferFormat;
	skyPsoDesc.SampleDesc.Count = 1;
	skyPsoDesc.SampleDesc.Quality = 0;
	skyPsoDesc.DSVFormat = mDepthStencilFormat;

	// The camera is inside the sky sphere, so just turn off culling.
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// Make sure the depth function is LESS_EQUAL and not just LESS.  
	// Otherwise, the normalized depth values at z = 1 (NDC) will 
	// fail the depth test if the depth buffer was cleared to 1.
	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	skyPsoDesc.pRootSignature = mRootSignatures["Sky"].Get();
	skyPsoDesc.VS = GShaderManager::LoadShader(L"Shaders\\SkyVS.cso");
	skyPsoDesc.PS = GShaderManager::LoadShader(L"Shaders\\SkyPS.cso");
	/*
	skyPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyVS"]->GetBufferPointer()),
		mShaders["skyVS"]->GetBufferSize()
	};
	skyPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyPS"]->GetBufferPointer()),
		mShaders["skyPS"]->GetBufferSize()
	};
	*/
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs["Sky"])));



	//
	// PSO for irradiance pre-integration.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC irradiancePsoDesc;

	ZeroMemory(&irradiancePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	irradiancePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	irradiancePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	irradiancePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	irradiancePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	irradiancePsoDesc.SampleMask = UINT_MAX;
	irradiancePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	irradiancePsoDesc.NumRenderTargets = 1;
	irradiancePsoDesc.RTVFormats[0] = mCubeRtvs["Irradiance"]->mFormat;
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
	irradiancePsoDesc.VS = GShaderManager::LoadShader(L"Shaders\\SkyVS.cso");
	irradiancePsoDesc.PS = GShaderManager::LoadShader(L"Shaders\\IrradianceCubemapPS.cso");
	/*
	skyPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyVS"]->GetBufferPointer()),
		mShaders["skyVS"]->GetBufferSize()
	};
	skyPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyPS"]->GetBufferPointer()),
		mShaders["skyPS"]->GetBufferSize()
	};
	*/
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&irradiancePsoDesc, IID_PPV_ARGS(&mPSOs["Irradiance"])));

	//
	// PSO for prefilter pre-integration.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC prefilterPsoDesc;

	ZeroMemory(&prefilterPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	prefilterPsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	prefilterPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	prefilterPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	prefilterPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	prefilterPsoDesc.SampleMask = UINT_MAX;
	prefilterPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	prefilterPsoDesc.NumRenderTargets = 1;
	prefilterPsoDesc.RTVFormats[0] = mCubeRtvs["Prefilter_0"]->mFormat;
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
	prefilterPsoDesc.VS = GShaderManager::LoadShader(L"Shaders\\SkyVS.cso");
	prefilterPsoDesc.PS = GShaderManager::LoadShader(L"Shaders\\PrefilterCubemapPS.cso");
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&prefilterPsoDesc, IID_PPV_ARGS(&mPSOs["Prefilter"])));

}

void GCore::BuildFrameResources()
{
	for (int i = 0; i < NUM_FRAME_RESOURCES; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			2, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
	}
}

void GCore::BuildMaterials()
{
	auto defaultMat = std::make_unique<GMaterial>();
	defaultMat->Name = "default";
	defaultMat->MatCBIndex = 0;
	defaultMat->mTextures.push_back(mTextures["sphere_1_BaseColor"]);
	defaultMat->mTextures.push_back(mTextures["sphere_1_Normal"]);
	defaultMat->mTextures.push_back(mTextures["sphere_1_OcclusionRoughnessMetallic"]);
	mMaterials["default"] = std::move(defaultMat);

	auto debug_albedo = std::make_unique<GMaterial>();
	debug_albedo->Name = "debug_albedo";
	debug_albedo->MatCBIndex = 1;
	debug_albedo->ScalarParams.push_back(0.01f);//Albedo
	debug_albedo->ScalarParams.push_back(0.01f);//RGB
	mMaterials["debug_albedo"] = std::move(debug_albedo);

	auto debug_normal = std::make_unique<GMaterial>();
	debug_normal->Name = "debug_normal";
	debug_normal->MatCBIndex = 2;
	debug_normal->ScalarParams.push_back(1.01f);//Normal
	debug_normal->ScalarParams.push_back(0.01f);//RGB
	mMaterials["debug_normal"] = std::move(debug_normal);

	auto debug_worldpos = std::make_unique<GMaterial>();
	debug_worldpos->Name = "debug_worldpos";
	debug_worldpos->MatCBIndex = 3;
	debug_worldpos->ScalarParams.push_back(2.01f);//WorldPos
	debug_worldpos->ScalarParams.push_back(0.01f);//RGB
	mMaterials["debug_worldpos"] = std::move(debug_worldpos);

	auto debug_roughness = std::make_unique<GMaterial>();
	debug_roughness->Name = "debug_roughness";
	debug_roughness->MatCBIndex = 4;
	debug_roughness->ScalarParams.push_back(3.01f);//OcclusionRoughnessMetallic
	debug_roughness->ScalarParams.push_back(2.01f);//Green
	mMaterials["debug_roughness"] = std::move(debug_roughness);

	auto debug_metallic = std::make_unique<GMaterial>();
	debug_metallic->Name = "debug_metallic";
	debug_metallic->MatCBIndex = 5;
	debug_metallic->ScalarParams.push_back(3.01f);//OcclusionRoughnessMetallic
	debug_metallic->ScalarParams.push_back(3.01f);//Blue
	mMaterials["debug_metallic"] = std::move(debug_metallic);

	auto sphere_1 = std::make_unique<GMaterial>();
	sphere_1->Name = "sphere_1";
	sphere_1->MatCBIndex = 6;
	sphere_1->mTextures.push_back(mTextures["sphere_1_BaseColor"]);
	sphere_1->mTextures.push_back(mTextures["sphere_1_Normal"]);
	sphere_1->mTextures.push_back(mTextures["sphere_1_OcclusionRoughnessMetallic"]);
	mMaterials["sphere_1"] = std::move(sphere_1);

	auto sphere_2 = std::make_unique<GMaterial>();
	sphere_2->Name = "sphere_2";
	sphere_2->MatCBIndex = 7;
	sphere_2->mTextures.push_back(mTextures["sphere_2_BaseColor"]);
	sphere_2->mTextures.push_back(mTextures["sphere_2_Normal"]);
	sphere_2->mTextures.push_back(mTextures["sphere_2_OcclusionRoughnessMetallic"]);
	mMaterials["sphere_2"] = std::move(sphere_2);

	auto bricks = std::make_unique<GMaterial>();
	bricks->Name = "bricks";
	bricks->MatCBIndex = 8;
	bricks->mTextures.push_back(mTextures["bricksDiffuseMap"]);//Diffuse
	bricks->mTextures.push_back(mTextures["bricksNormalMap"]);//Normal
	bricks->VectorParams.push_back(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));//DiffuseAlbedo
	bricks->VectorParams.push_back(XMFLOAT4(0.1f, 0.1f, 0.1f, 0.f));//FresnelR0
	bricks->ScalarParams.push_back(0.3f);//Roughness
	mMaterials["bricks"] = std::move(bricks);

	auto tile = std::make_unique<GMaterial>();
	tile->Name = "tile";
	tile->MatCBIndex = 9;
	tile->mTextures.push_back(mTextures["tileDiffuseMap"]);//Diffuse
	tile->mTextures.push_back(mTextures["tileNormalMap"]);//Normal
	tile->VectorParams.push_back(XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f));//DiffuseAlbedo
	tile->VectorParams.push_back(XMFLOAT4(0.2f, 0.2f, 0.2f, 0.f));//FresnelR0
	tile->ScalarParams.push_back(0.1f);//Roughness
	mMaterials["tile"] = std::move(tile);

	auto mirror = std::make_unique<GMaterial>();
	mirror->Name = "mirror";
	mirror->MatCBIndex = 10;
	mirror->mTextures.push_back(mTextures["defaultDiffuseMap"]);//Diffuse
	mirror->mTextures.push_back(mTextures["defaultNormalMap"]);//Normal
	mirror->VectorParams.push_back(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));//DiffuseAlbedo
	mirror->VectorParams.push_back(XMFLOAT4(0.98f, 0.97f, 0.95f, 0.f));//FresnelR0
	mirror->ScalarParams.push_back(0.1f);//Roughness
	mMaterials["mirror"] = std::move(mirror);

	auto skull = std::make_unique<GMaterial>();
	skull->Name = "skull";
	skull->MatCBIndex = 11;
	skull->mTextures.push_back(mTextures["defaultDiffuseMap"]);//Diffuse
	skull->mTextures.push_back(mTextures["defaultNormalMap"]);//Normal
	skull->VectorParams.push_back(XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));//DiffuseAlbedo
	skull->VectorParams.push_back(XMFLOAT4(0.6f, 0.6f, 0.6f, 0.f));//FresnelR0
	skull->ScalarParams.push_back(0.2f);//Roughness
	mMaterials["skull"] = std::move(skull);

	auto sky = std::make_unique<GMaterial>();
	sky->Name = "sky";
	sky->MatCBIndex = 12;
	//sky->DiffuseSrvHeapIndex = 6;
	//sky->NormalSrvHeapIndex = 7;
	sky->mTextures.push_back(mTextures["defaultDiffuseMap"]);//Diffuse
	sky->mTextures.push_back(mTextures["defaultNormalMap"]);//Normal
	sky->VectorParams.push_back(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));//DiffuseAlbedo
	sky->VectorParams.push_back(XMFLOAT4(0.1f, 0.1f, 0.1f, 0.f));//FresnelR0
	sky->ScalarParams.push_back(1.0f);//Roughness
	mMaterials["sky"] = std::move(sky);

	auto greasyPanMat = std::make_unique<GMaterial>();
	greasyPanMat->Name = "GreasyPan";
	greasyPanMat->MatCBIndex = 13;
	greasyPanMat->mTextures.push_back(mTextures["Greasy_Pan_Albedo"]);
	greasyPanMat->mTextures.push_back(mTextures["Greasy_Pan_Normal"]);
	greasyPanMat->mTextures.push_back(mTextures["Greasy_Pan_Orm"]);
	mMaterials["GreasyPan"] = std::move(greasyPanMat);

	auto rustedIronMat = std::make_unique<GMaterial>();
	rustedIronMat->Name = "RustedIron";
	rustedIronMat->MatCBIndex = 14;
	rustedIronMat->mTextures.push_back(mTextures["Rusted_Iron_Albedo"]);
	rustedIronMat->mTextures.push_back(mTextures["Rusted_Iron_Normal"]);
	rustedIronMat->mTextures.push_back(mTextures["Rusted_Iron_Orm"]);
	mMaterials["RustedIron"] = std::move(rustedIronMat);

	auto cerberusMat = std::make_unique<GMaterial>();
	cerberusMat->Name = "Cerberus";
	cerberusMat->MatCBIndex = 15;
	cerberusMat->mTextures.push_back(mTextures["Cerberus_Albedo"]);
	cerberusMat->mTextures.push_back(mTextures["Cerberus_Normal"]);
	cerberusMat->mTextures.push_back(mTextures["Cerberus_Orm"]);
	mMaterials["Cerberus"] = std::move(cerberusMat);

	auto fireplaceMat = std::make_unique<GMaterial>();
	fireplaceMat->Name = "Fireplace";
	fireplaceMat->MatCBIndex = 16;
	fireplaceMat->mTextures.push_back(mTextures["Fireplace_Albedo"]);
	fireplaceMat->mTextures.push_back(mTextures["Fireplace_Normal"]);
	fireplaceMat->mTextures.push_back(mTextures["Fireplace_Orm"]);
	mMaterials["Fireplace"] = std::move(fireplaceMat);

}

void GCore::BuildSceneObjects()
{
	UINT indexCB = 0;

	// Create screen quads for light pass and post process.
	{
		auto fullScreenQuadRitem = std::make_shared<GSceneObject>();
		//fullScreenQuadRitem->World = MathHelper::Identity4x4();
		//XMStoreFloat4x4(&fullScreenQuadRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f)*XMMatrixTranslation(0.0f, 0.0f, 0.0f));
		fullScreenQuadRitem->TexTransform = MathHelper::Identity4x4();
		fullScreenQuadRitem->ObjCBIndex = indexCB;
		fullScreenQuadRitem->Mat = mMaterials["default"];
		fullScreenQuadRitem->Mesh = mMeshes["Quad"];
		fullScreenQuadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		//fullScreenQuadRitem->SubmeshName = "quad";
		//fullScreenQuadRitem->IndexCount = fullScreenQuadRitem->Mesh->Submeshes["quad"].IndexCount;
		//fullScreenQuadRitem->StartIndexLocation = fullScreenQuadRitem->Mesh->Submeshes["quad"].StartIndexLocation;
		//fullScreenQuadRitem->BaseVertexLocation = fullScreenQuadRitem->Mesh->Submeshes["quad"].BaseVertexLocation;
		mSceneObjectLayer[(int)RenderLayer::ScreenQuad].push_back(fullScreenQuadRitem);
		mAllRitems.push_back(std::move(fullScreenQuadRitem));
		indexCB++;
	}

	auto skyRitem = std::make_shared<GSceneObject>();
	//XMStoreFloat4x4(&skyRitem->World, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
	skyRitem->SetScale(5000.f, 5000.f, 5000.f);
	//skyRitem->TexTransform = MathHelper::Identity4x4();
	skyRitem->ObjCBIndex = indexCB;
	skyRitem->Mat = mMaterials["sky"];
	skyRitem->Mesh = mMeshes["Sphere"];
	skyRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//skyRitem->SubmeshName = "sphere";
	//skyRitem->IndexCount = skyRitem->Mesh->Submeshes["sphere"].IndexCount;
	//skyRitem->StartIndexLocation = skyRitem->Mesh->Submeshes["sphere"].StartIndexLocation;
	//skyRitem->BaseVertexLocation = skyRitem->Mesh->Submeshes["sphere"].BaseVertexLocation;
	mSceneObjectLayer[(int)RenderLayer::Sky].push_back(skyRitem);
	mAllRitems.push_back(std::move(skyRitem));
	indexCB++;

	// Create debug quads.
	{
		auto albedoQuadRitem = std::make_shared<GSceneObject>();
		//albedoQuadRitem->World = MathHelper::Identity4x4();
		//XMStoreFloat4x4(&albedoQuadRitem->World, XMMatrixScaling(0.2f, 0.2f, 0.2f)*XMMatrixTranslation(0.0f, 0.0f, 0.0f));
		albedoQuadRitem->SetScale(.2f, .2f, .2f);
		albedoQuadRitem->SetLocation(0.f, 0.f, 0.f);
		albedoQuadRitem->TexTransform = MathHelper::Identity4x4();
		albedoQuadRitem->ObjCBIndex = indexCB;
		albedoQuadRitem->Mat = mMaterials["debug_albedo"];
		albedoQuadRitem->Mesh = mMeshes["Quad"];
		albedoQuadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		//albedoQuadRitem->SubmeshName = "quad";
		//albedoQuadRitem->IndexCount = albedoQuadRitem->Mesh->Submeshes["quad"].IndexCount;
		//albedoQuadRitem->StartIndexLocation = albedoQuadRitem->Mesh->Submeshes["quad"].StartIndexLocation;
		//albedoQuadRitem->BaseVertexLocation = albedoQuadRitem->Mesh->Submeshes["quad"].BaseVertexLocation;
		mSceneObjectLayer[(int)RenderLayer::Debug].push_back(albedoQuadRitem);
		mAllRitems.push_back(std::move(albedoQuadRitem));
		indexCB++;

		auto normalQuadRitem = std::make_shared<GSceneObject>();
		//normalQuadRitem->World = MathHelper::Identity4x4();
		//XMStoreFloat4x4(&normalQuadRitem->World, XMMatrixScaling(0.2f, 0.2f, 0.2f)*XMMatrixTranslation(0.2f, 0.0f, 0.0f));
		normalQuadRitem->SetScale(.2f, .2f, .2f);
		normalQuadRitem->SetLocation(.2f, 0.f, 0.f);
		normalQuadRitem->TexTransform = MathHelper::Identity4x4();
		normalQuadRitem->ObjCBIndex = indexCB;
		normalQuadRitem->Mat = mMaterials["debug_normal"];
		normalQuadRitem->Mesh = mMeshes["Quad"];
		normalQuadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		//normalQuadRitem->SubmeshName = "quad";
		//normalQuadRitem->IndexCount = normalQuadRitem->Mesh->Submeshes["quad"].IndexCount;
		//normalQuadRitem->StartIndexLocation = normalQuadRitem->Mesh->Submeshes["quad"].StartIndexLocation;
		//normalQuadRitem->BaseVertexLocation = normalQuadRitem->Mesh->Submeshes["quad"].BaseVertexLocation;
		mSceneObjectLayer[(int)RenderLayer::Debug].push_back(normalQuadRitem);
		mAllRitems.push_back(std::move(normalQuadRitem));
		indexCB++;

		auto worldposQuadRitem = std::make_shared<GSceneObject>();
		//worldposQuadRitem->World = MathHelper::Identity4x4();
		//XMStoreFloat4x4(&worldposQuadRitem->World, XMMatrixScaling(0.2f, 0.2f, 0.2f)*XMMatrixTranslation(0.4f, 0.0f, 0.0f));
		worldposQuadRitem->SetScale(.2f, .2f, .2f);
		worldposQuadRitem->SetLocation(.4f, 0.f, 0.f);
		worldposQuadRitem->TexTransform = MathHelper::Identity4x4();
		worldposQuadRitem->ObjCBIndex = indexCB;
		worldposQuadRitem->Mat = mMaterials["debug_worldpos"];
		worldposQuadRitem->Mesh = mMeshes["Quad"];
		worldposQuadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		//worldposQuadRitem->SubmeshName = "quad";
		//worldposQuadRitem->IndexCount = worldposQuadRitem->Mesh->Submeshes["quad"].IndexCount;
		//worldposQuadRitem->StartIndexLocation = worldposQuadRitem->Mesh->Submeshes["quad"].StartIndexLocation;
		//worldposQuadRitem->BaseVertexLocation = worldposQuadRitem->Mesh->Submeshes["quad"].BaseVertexLocation;
		mSceneObjectLayer[(int)RenderLayer::Debug].push_back(worldposQuadRitem);
		mAllRitems.push_back(std::move(worldposQuadRitem));
		indexCB++;

		auto roughnessQuadRitem = std::make_shared<GSceneObject>();
		//roughnessQuadRitem->World = MathHelper::Identity4x4();
		//XMStoreFloat4x4(&roughnessQuadRitem->World, XMMatrixScaling(0.2f, 0.2f, 0.2f)*XMMatrixTranslation(0.6f, 0.0f, 0.0f));
		roughnessQuadRitem->SetScale(.2f, .2f, .2f);
		roughnessQuadRitem->SetLocation(.6f, 0.f, 0.f);
		roughnessQuadRitem->TexTransform = MathHelper::Identity4x4();
		roughnessQuadRitem->ObjCBIndex = indexCB;
		roughnessQuadRitem->Mat = mMaterials["debug_roughness"];
		roughnessQuadRitem->Mesh = mMeshes["Quad"];
		roughnessQuadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		//roughnessQuadRitem->SubmeshName = "quad";
		//roughnessQuadRitem->IndexCount = roughnessQuadRitem->Mesh->Submeshes["quad"].IndexCount;
		//roughnessQuadRitem->StartIndexLocation = roughnessQuadRitem->Mesh->Submeshes["quad"].StartIndexLocation;
		//roughnessQuadRitem->BaseVertexLocation = roughnessQuadRitem->Mesh->Submeshes["quad"].BaseVertexLocation;
		mSceneObjectLayer[(int)RenderLayer::Debug].push_back(roughnessQuadRitem);
		mAllRitems.push_back(std::move(roughnessQuadRitem));
		indexCB++;

		auto metallicQuadRitem = std::make_shared<GSceneObject>();
		//metallicQuadRitem->World = MathHelper::Identity4x4();
		//XMStoreFloat4x4(&metallicQuadRitem->World, XMMatrixScaling(0.2f, 0.2f, 0.2f)*XMMatrixTranslation(0.8f, 0.0f, 0.0f));
		metallicQuadRitem->SetScale(.2f, .2f, .2f);
		metallicQuadRitem->SetLocation(.8f, 0.f, 0.f);
		metallicQuadRitem->TexTransform = MathHelper::Identity4x4();
		metallicQuadRitem->ObjCBIndex = indexCB;
		metallicQuadRitem->Mat = mMaterials["debug_metallic"];
		metallicQuadRitem->Mesh = mMeshes["Quad"];
		metallicQuadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		//metallicQuadRitem->SubmeshName = "quad";
		//metallicQuadRitem->IndexCount = metallicQuadRitem->Mesh->Submeshes["quad"].IndexCount;
		//metallicQuadRitem->StartIndexLocation = metallicQuadRitem->Mesh->Submeshes["quad"].StartIndexLocation;
		//metallicQuadRitem->BaseVertexLocation = metallicQuadRitem->Mesh->Submeshes["quad"].BaseVertexLocation;
		mSceneObjectLayer[(int)RenderLayer::Debug].push_back(metallicQuadRitem);
		mAllRitems.push_back(std::move(metallicQuadRitem));
		indexCB++;
	}

	auto cerberusRitem = std::make_shared<GSceneObject>();
	//XMStoreFloat4x4(&cerberusRitem->World, XMMatrixScaling(0.1f, 0.1f, 0.1f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f));
	//cerberusRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&cerberusRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	cerberusRitem->ObjCBIndex = indexCB;
	cerberusRitem->Mat = mMaterials["Cerberus"];
	cerberusRitem->Mesh = mMeshes["Cerberus"];
	cerberusRitem->Name = "Cerberus";
	cerberusRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//cerberusRitem->SubmeshName = submesh.first;
	mSceneObjectLayer[(int)RenderLayer::Deferred].push_back(cerberusRitem);
	mAllRitems.push_back(std::move(cerberusRitem));
	indexCB++;

	auto sphereRitem = std::make_shared<GSceneObject>();
	sphereRitem->SetScale(20.f, 20.f, 20.f);
	sphereRitem->SetLocation(0.f, -100.f, 0.f);
	XMStoreFloat4x4(&sphereRitem->TexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
	sphereRitem->ObjCBIndex = indexCB;
	sphereRitem->Mat = mMaterials["sphere_2"];
	sphereRitem->Mesh = mMeshes["Sphere"];
	sphereRitem->Name = "Sphere_1";
	sphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mSceneObjectLayer[(int)RenderLayer::Deferred].push_back(sphereRitem);
	mAllRitems.push_back(std::move(sphereRitem));
	indexCB++;

	auto sphereRitem2 = std::make_shared<GSceneObject>();
	sphereRitem2->SetScale(20.f, 20.f, 20.f);
	sphereRitem2->SetLocation(0.f, -100.f, 0.f);
	XMStoreFloat4x4(&sphereRitem2->TexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
	sphereRitem2->ObjCBIndex = indexCB;
	sphereRitem2->Mat = mMaterials["GreasyPan"];
	sphereRitem2->Mesh = mMeshes["Sphere"];
	sphereRitem2->Name = "Sphere_2";
	sphereRitem2->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mSceneObjectLayer[(int)RenderLayer::Deferred].push_back(sphereRitem2);
	mAllRitems.push_back(std::move(sphereRitem2));
	indexCB++;

	/*
	for (auto submesh : mMeshes["Fireplace"]->Submeshes)
	{
		auto fireplaceRitem = std::make_shared<GRenderObject>();
		DirectX::XMFLOAT4 xAxis = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
		XMStoreFloat4x4(&fireplaceRitem->World, XMMatrixScaling(0.1f, 0.1f, 0.1f) * XMMatrixTranslation(0.0f, 5.0f, 0.0f));
		fireplaceRitem->World = MathHelper::Identity4x4();
		XMStoreFloat4x4(&fireplaceRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
		fireplaceRitem->ObjCBIndex = indexCB;
		fireplaceRitem->Mat = mMaterials["Fireplace"];
		fireplaceRitem->Mesh = mMeshes["Fireplace"];
		fireplaceRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		fireplaceRitem->SubmeshName = submesh.first;
		mRitemLayer[(int)RenderLayer::Deferred].push_back(fireplaceRitem.get());
		mAllRitems.push_back(std::move(fireplaceRitem));
		indexCB++;
	}
	*/

	/*
	auto sphereRitem_3 = std::make_shared<GRenderObject>();
	XMStoreFloat4x4(&sphereRitem_3->World, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(2.3f, 1.0f, 0.0f));
	XMStoreFloat4x4(&sphereRitem_3->TexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
	sphereRitem_3->ObjCBIndex = 8;
	sphereRitem_3->Mat = mMaterials["sphere_1"];
	sphereRitem_3->Mesh = mMeshes["shapeGeo"];
	sphereRitem_3->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	sphereRitem_3->SubmeshName = "sphere";
	//sphereRitem_3->IndexCount = sphereRitem_3->Mesh->Submeshes["sphere"].IndexCount;
	//sphereRitem_3->StartIndexLocation = sphereRitem_3->Mesh->Submeshes["sphere"].StartIndexLocation;
	//sphereRitem_3->BaseVertexLocation = sphereRitem_3->Mesh->Submeshes["sphere"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Deferred].push_back(sphereRitem_3.get());
	mAllRitems.push_back(std::move(sphereRitem_3));
	*/

	/*
	auto sphereRitem_4 = std::make_shared<GRenderObject>();
	XMStoreFloat4x4(&sphereRitem_4->World, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(-2.3f, 1.0f, 0.0f));
	XMStoreFloat4x4(&sphereRitem_4->TexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
	sphereRitem_4->ObjCBIndex = 9;
	sphereRitem_4->Mat = mMaterials["sphere_2"];
	sphereRitem_4->Mesh = mMeshes["shapeGeo"];
	sphereRitem_4->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	sphereRitem_4->SubmeshName = "sphere";
	//sphereRitem_4->IndexCount = sphereRitem_4->Mesh->Submeshes["sphere"].IndexCount;
	//sphereRitem_4->StartIndexLocation = sphereRitem_4->Mesh->Submeshes["sphere"].StartIndexLocation;
	//sphereRitem_4->BaseVertexLocation = sphereRitem_4->Mesh->Submeshes["sphere"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Deferred].push_back(sphereRitem_4.get());
	mAllRitems.push_back(std::move(sphereRitem_4));
	*/

	/*
	auto quadRitem = std::make_unique<GMesh>();
	quadRitem->World = MathHelper::Identity4x4();
	quadRitem->TexTransform = MathHelper::Identity4x4();
	quadRitem->ObjCBIndex = 1;
	quadRitem->Mat = mMaterials["bricks"];
	quadRitem->Geometry = mGeometries["shapeGeo"].get();
	quadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	quadRitem->IndexCount = quadRitem->Geometry->DrawArgs["quad"].IndexCount;
	quadRitem->StartIndexLocation = quadRitem->Geometry->DrawArgs["quad"].StartIndexLocation;
	quadRitem->BaseVertexLocation = quadRitem->Geometry->DrawArgs["quad"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Debug].push_back(quadRitem.get());
	mAllRitems.push_back(std::move(quadRitem));
	*/

	/*
	auto boxRitem = std::make_unique<GMesh>();
	XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0f, 1.0f, 2.0f)*XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
	boxRitem->ObjCBIndex = 2;
	boxRitem->Mat = mMaterials["bricks"];
	boxRitem->Geometry = mGeometries["shapeGeo"].get();
	boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geometry->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geometry->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geometry->DrawArgs["box"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
	mAllRitems.push_back(std::move(boxRitem));

	auto skullRitem = std::make_unique<GMesh>();
	XMStoreFloat4x4(&skullRitem->World, XMMatrixScaling(0.4f, 0.4f, 0.4f)*XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	skullRitem->TexTransform = MathHelper::Identity4x4();
	skullRitem->ObjCBIndex = 3;
	skullRitem->Mat = mMaterials["skull"];
	skullRitem->Geometry = mGeometries["skullGeo"].get();
	skullRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->IndexCount = skullRitem->Geometry->DrawArgs["skull"].IndexCount;
	skullRitem->StartIndexLocation = skullRitem->Geometry->DrawArgs["skull"].StartIndexLocation;
	skullRitem->BaseVertexLocation = skullRitem->Geometry->DrawArgs["skull"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(skullRitem.get());
	mAllRitems.push_back(std::move(skullRitem));

	auto gridRitem = std::make_unique<GMesh>();
	gridRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
	gridRitem->ObjCBIndex = 4;
	gridRitem->Mat = mMaterials["tile"];
	gridRitem->Geometry = mGeometries["shapeGeo"].get();
	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geometry->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geometry->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geometry->DrawArgs["grid"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
	mAllRitems.push_back(std::move(gridRitem));

	XMMATRIX brickTexTransform = XMMatrixScaling(1.5f, 2.0f, 1.0f);
	UINT objCBIndex = 5;
	for (int i = 0; i < 5; ++i)
	{
		auto leftCylRitem = std::make_unique<GMesh>();
		auto rightCylRitem = std::make_unique<GMesh>();
		auto leftSphereRitem = std::make_unique<GMesh>();
		auto rightSphereRitem = std::make_unique<GMesh>();

		XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);

		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

		XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
		XMStoreFloat4x4(&leftCylRitem->TexTransform, brickTexTransform);
		leftCylRitem->ObjCBIndex = objCBIndex++;
		leftCylRitem->Mat = mMaterials["bricks"];
		leftCylRitem->Geometry = mGeometries["shapeGeo"].get();
		leftCylRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->IndexCount = leftCylRitem->Geometry->DrawArgs["cylinder"].IndexCount;
		leftCylRitem->StartIndexLocation = leftCylRitem->Geometry->DrawArgs["cylinder"].StartIndexLocation;
		leftCylRitem->BaseVertexLocation = leftCylRitem->Geometry->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
		XMStoreFloat4x4(&rightCylRitem->TexTransform, brickTexTransform);
		rightCylRitem->ObjCBIndex = objCBIndex++;
		rightCylRitem->Mat = mMaterials["bricks"];
		rightCylRitem->Geometry = mGeometries["shapeGeo"].get();
		rightCylRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geometry->DrawArgs["cylinder"].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geometry->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geometry->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
		leftSphereRitem->TexTransform = MathHelper::Identity4x4();
		leftSphereRitem->ObjCBIndex = objCBIndex++;
		leftSphereRitem->Mat = mMaterials["mirror"];
		leftSphereRitem->Geometry = mGeometries["shapeGeo"].get();
		leftSphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geometry->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geometry->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geometry->DrawArgs["sphere"].BaseVertexLocation;

		XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
		rightSphereRitem->TexTransform = MathHelper::Identity4x4();
		rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Mat = mMaterials["mirror"];
		rightSphereRitem->Geometry = mGeometries["shapeGeo"].get();
		rightSphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->IndexCount = rightSphereRitem->Geometry->DrawArgs["sphere"].IndexCount;
		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geometry->DrawArgs["sphere"].StartIndexLocation;
		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geometry->DrawArgs["sphere"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::Opaque].push_back(leftCylRitem.get());
		mRitemLayer[(int)RenderLayer::Opaque].push_back(rightCylRitem.get());
		mRitemLayer[(int)RenderLayer::Opaque].push_back(leftSphereRitem.get());
		mRitemLayer[(int)RenderLayer::Opaque].push_back(rightSphereRitem.get());

		mAllRitems.push_back(std::move(leftCylRitem));
		mAllRitems.push_back(std::move(rightCylRitem));
		mAllRitems.push_back(std::move(leftSphereRitem));
		mAllRitems.push_back(std::move(rightSphereRitem));
	}
	*/
}

/*
void GCore::DrawSceneObjects(ID3D12GraphicsCommandList* cmdList, const std::vector<GSceneObject*>& objects)
{
	// For each render item...
	for (size_t i = 0; i < objects.size(); ++i)
	{
		DrawSceneObject(cmdList, objects[i]);
	}
}
*/

void GCore::DrawSceneObjects(ID3D12GraphicsCommandList* cmdList, const RenderLayer layer, bool bSetCBV)
{
	// For each render item...
	for (size_t i = 0; i < mSceneObjectLayer[((int)layer)].size(); ++i)
	{
		auto sObject = mSceneObjectLayer[((int)layer)][i];
		DrawSceneObject(cmdList, sObject.get(), bSetCBV);
	}
}

void GCore::DrawSceneObject(ID3D12GraphicsCommandList* cmdList, const GSceneObject* sObject, bool bSetCBV)
{

	cmdList->IASetVertexBuffers(0, 1, &sObject->Mesh->mVIBuffer->VertexBufferView());
	cmdList->IASetIndexBuffer(&sObject->Mesh->mVIBuffer->IndexBufferView());
	cmdList->IASetPrimitiveTopology(sObject->PrimitiveType);

	if (bSetCBV)
	{
		UINT objCBByteSize = GDX12Util::CalcConstantBufferByteSize(sizeof(ObjectConstants));
		auto objectCB = mCurrFrameResource->ObjectCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + sObject->ObjCBIndex * objCBByteSize;
		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
	}

	cmdList->DrawIndexedInstanced(sObject->Mesh->mVIBuffer->IndexCount, 1, 0, 0, 0);
}

void GCore::DrawSceneToShadowMap()
{
	mCommandList->RSSetViewports(1, &mShadowMap->Viewport());
	mCommandList->RSSetScissorRects(1, &mShadowMap->ScissorRect());

	// Change to DEPTH_WRITE.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearDepthStencilView(mShadowMap->Dsv(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(0, nullptr, false, &mShadowMap->Dsv());

	// Bind the pass constant buffer for the shadow map pass.
	UINT passCBByteSize = GDX12Util::CalcConstantBufferByteSize(sizeof(PassConstants));
	auto passCB = mCurrFrameResource->PassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1 * passCBByteSize;
	mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

	mCommandList->SetPipelineState(mPSOs["shadow_opaque"].Get());

	DrawSceneObjects(mCommandList.Get(), RenderLayer::Opaque, true);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void GCore::DrawNormalsAndDepth()
{
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	auto normalMap = mSsao->NormalMap();
	auto normalMapRtv = mSsao->NormalMapRtv();

	// Change to RENDER_TARGET.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the screen normal map and depth buffer.
	float clearValue[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	mCommandList->ClearRenderTargetView(normalMapRtv, clearValue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &normalMapRtv, true, &DepthStencilView());

	// Bind the constant buffer for this pass.
	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	mCommandList->SetPipelineState(mPSOs["drawNormals"].Get());

	DrawSceneObjects(mCommandList.Get(), RenderLayer::Opaque, true);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

CD3DX12_CPU_DESCRIPTOR_HANDLE GCore::GetCpuSrv(int index)const
{
	auto srv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	srv.Offset(index, mCbvSrvUavDescriptorSize);
	return srv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE GCore::GetGpuSrv(int index)const
{
	auto srv = CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	srv.Offset(index, mCbvSrvUavDescriptorSize);
	return srv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE GCore::GetDsv(int index)const
{
	auto dsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
	dsv.Offset(index, mDsvDescriptorSize);
	return dsv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE GCore::GetRtv(int index)const
{
	auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	rtv.Offset(index, mRtvDescriptorSize);
	return rtv;
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GCore::GetStaticSamplers()
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

#pragma region export

int GCore::GetSceneObjectNum()
{
	return (int)(mSceneObjectLayer[(int)RenderLayer::Deferred].size());
}

const char* GCore::GetSceneObjectName(int index)
{
	//char* cstr = new char[256];
	//strcpy_s(cstr, 256, mSceneObjectLayer[(int)RenderLayer::Deferred][index]->Name.c_str());
	//return cstr;
	return mSceneObjectLayer[(int)RenderLayer::Deferred][index]->Name.c_str();
}

void GCore::SetSetSceneObjectsCallback(VoidFuncPointerType pSetSceneObjectsCallback)
{
	mSetSceneObjectsCallback = pSetSceneObjectsCallback;
}

void GCore::SetSceneObjectsCallback()
{
	mSetSceneObjectsCallback();
}

void GCore::GetSceneObjectTransform(char* objName, float* trans)
{
	std::string sObjectName(objName);
	for (auto sObject : mAllRitems)
	{
		if (sObject->Name == sObjectName)
		{
			XMFLOAT3 loc = sObject->GetLocation();
			XMFLOAT3 rot = sObject->GetRotation();
			XMFLOAT3 scale = sObject->GetScale();
			trans[0] = loc.x;
			trans[1] = loc.y;
			trans[2] = loc.z;
			trans[3] = rot.x;
			trans[4] = rot.y;
			trans[5] = rot.z;
			trans[6] = scale.x;
			trans[7] = scale.y;
			trans[8] = scale.z;
			return;
		}
	}
}

void GCore::SetSceneObjectTransform(char* objName, float* trans)
{
	std::string sObjectName(objName);
	for (auto sObject : mAllRitems)
	{
		if (sObject->Name == sObjectName)
		{
			sObject->SetLocation(trans[0], trans[1], trans[2]);
			sObject->SetRotation(trans[3], trans[4], trans[5]);
			sObject->SetScale(trans[6], trans[7], trans[8]);
			return;
		}
	}
}

#pragma endregion