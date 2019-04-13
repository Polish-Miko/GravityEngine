#pragma once
#include "stdafx.h"
#include "GCore.h"
#include <WindowsX.h>

#include <io.h>

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

#pragma region Initialize

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

		SetWorkDirectory();

		// Reset the command list to prep for initialization commands.
		ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

		mCamera.SetPosition(0.0f, 2.0f, -5.0f);
		/*
		mShadowMap = std::make_unique<ShadowMap>(md3dDevice.Get(),
			2048, 2048);

		mSsao = std::make_unique<Ssao>(
			md3dDevice.Get(),
			mCommandList.Get(),
			mClientWidth, mClientHeight);
		*/
		BuildCubemapSampleCameras();
		LoadTextures();
		BuildDescriptorHeaps();
		BuildRootSignature();
		BuildShadersAndInputLayout();
		LoadMeshes();
		BuildMaterials();
		BuildSceneObjects();
		BuildFrameResources();
		BuildPSOs();

		CubemapPreIntegration();

		//mSsao->SetPSOs(mPSOs["ssao"].Get(), mPSOs["ssaoBlur"].Get());
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

#pragma endregion

#pragma region MsgProc

void GCore::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
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
}

#pragma endregion

#pragma region Update

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
	//UpdateShadowPassCB(gt);
	//UpdateSsaoCB(gt);
	UpdateLightCB(gt);
}

#pragma endregion

#pragma region Draw

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

			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRtvHeaps["LightPass"]->mRtv[1]->mResource.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
		}

		// Debug Pass
		bool bDrawDebugQuad = true;
		if (bDrawDebugQuad)
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

#pragma endregion

#pragma region Init

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

void GCore::SetWorkDirectory()
{
	TCHAR exeFullPath[MAX_PATH];
	memset(exeFullPath, 0, MAX_PATH);

	GetModuleFileName(NULL, exeFullPath, MAX_PATH);
	WCHAR *p = wcsrchr(exeFullPath, '\\');
	*p = 0x00;

	WorkDirectory = std::wstring(exeFullPath);
}

#pragma endregion

#pragma region Util

std::vector<std::wstring> GCore::GetAllFilesInFolder(std::wstring relPath, bool bCheckFormat, std::vector<std::wstring> format)
{
	std::vector<std::wstring> files;
	long hFile = 0;
	struct _wfinddata_t fileinfo;
	relPath = WorkDirectory + relPath;
	std::wstring p;
	if ((hFile = _wfindfirst(p.assign(relPath).append(L"\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			if ((fileinfo.attrib &  _A_SUBDIR))
			{
				if (wcscmp(fileinfo.name, L".") != 0 && wcscmp(fileinfo.name, L"..") != 0)
				{
					std::vector<std::wstring> folderFiles = std::move(GetAllFilesInFolder(p.assign(relPath).append(L"\\").append(fileinfo.name), bCheckFormat, format));
					files.insert(files.end(), folderFiles.begin(), folderFiles.end());
				}
			}
			else
			{
				if (bCheckFormat)
				{
					bool isOfFormat = false;
					std::wstring sFileName(fileinfo.name);
					for (auto f : format)
					{
						if (sFileName.find(L"." + f) == (sFileName.length() - f.length() - 1))
						{
							isOfFormat = true;
							break;
						}
					}
					if (isOfFormat)
						files.push_back(p.assign(relPath).append(L"\\").append(fileinfo.name));
				}
				else
					files.push_back(p.assign(relPath).append(L"\\").append(fileinfo.name));
			}
		} while (_wfindnext(hFile, &fileinfo) == 0);

		_findclose(hFile);
	}
	return files;
}

std::vector<std::wstring> GCore::GetAllFilesUnderFolder(std::wstring relPath, bool bCheckFormat, std::vector<std::wstring> format)
{
	std::vector<std::wstring> files;
	long hFile = 0;
	struct _wfinddata_t fileinfo;
	relPath = WorkDirectory + relPath;
	std::wstring p;
	if ((hFile = _wfindfirst(p.assign(relPath).append(L"\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			if (!(fileinfo.attrib &  _A_SUBDIR))
			{
				if (bCheckFormat)
				{
					bool isOfFormat = false;
					std::wstring sFileName(fileinfo.name);
					for (auto f : format)
					{
						if (sFileName.find(L"." + f) == (sFileName.length() - f.length() - 1))
						{
							isOfFormat = true;
							break;
						}
					}
					if (isOfFormat)
						files.push_back(p.assign(relPath).append(L"\\").append(fileinfo.name));
				}
				else
					files.push_back(p.assign(relPath).append(L"\\").append(fileinfo.name));
			}
		} while (_wfindnext(hFile, &fileinfo) == 0);

		_findclose(hFile);
	}
	return files;
}

#pragma endregion

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