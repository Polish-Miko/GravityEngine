#pragma once
#include "stdafx.h"
#include "GCore.h"
#include <WindowsX.h>
#include <io.h>
#include "GDxFloat4x4.h"


GCore::GCore()
{
	mTimer = std::make_unique<GGiGameTimer>();
	mRenderer = &GDxRenderer::GetRenderer();
	mRenderer->SetTimer(mTimer.get());
}

GCore::~GCore()
{

}

GCore& GCore::GetCore()
{
	static GCore *instance = new GCore();
	return *instance;
}

void GCore::Run()
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try
	{
		try
		{
			MSG msg = { 0 };

			mTimer->Reset();

			while (msg.message != WM_QUIT)
			{
				// If there are Window messages then process them.
				if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				// Otherwise, do animation/game stuff.
				else
				{
					mTimer->Tick();

					if (!mAppPaused)
					{
						mRenderer->CalculateFrameStats();
						Update();
						mRenderer->Draw(mTimer.get());
					}
					else
					{
						Sleep(100);
					}
				}
			}
		}
		catch (DxException& e)
		{
			MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		}
	}
	catch (GGiException& e)
	{
		MessageBox(nullptr, e.GetErrorMessage().c_str(), L"Engine-defined Exception", MB_OK);
	}
}

void GCore::Initialize(HWND OutputWindow, double width, double height)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try
	{
		try
		{

			mRenderer->PreInitialize(OutputWindow, width, height);

			pRendererFactory = mRenderer->GetFactory();
			SetWorkDirectory();
			LoadTextures();
			mRenderer->SyncTextures(mTextures);
			LoadMaterials();
			mRenderer->SyncMaterials(mMaterials);

			mRenderer->Initialize(OutputWindow, width, height);

		}
		catch (DxException& e)
		{
			MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		}
	}
	catch (GGiException& e)
	{
		MessageBox(nullptr, e.GetErrorMessage().c_str(), L"Engine-defined Exception", MB_OK);
	}
}

void GCore::Update()
{
	mRenderer->Update(mTimer.get());
}

/*
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
*/

//MsgProc
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
			mTimer->Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer->Start();
		}
		return; 0;

		// WM_SIZE is sent when the user resizes the window.
	case WM_SIZE:
		// Save the new client area dimensions.
		mRenderer->SetClientWidth(LOWORD(lParam));
		mRenderer->SetClientHeight(HIWORD(lParam));
		if (mRenderer->IsRunning())
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
				mRenderer->OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{

				// Restoring from minimized state?
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					mRenderer->OnResize();
				}

				// Restoring from maximized state?
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					mRenderer->OnResize();
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
					mRenderer->OnResize();
				}
			}
		}
		return; 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		mTimer->Stop();
		return; 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		mTimer->Start();
		mRenderer->OnResize();
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
		mRenderer->OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return; 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		mRenderer->OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return; 0;
	case WM_MOUSEMOVE:
		mRenderer->OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return; 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		//else if ((int)wParam == VK_F2)
			//Set4xMsaaState(!m4xMsaaState);

		return; 0;
	}
}

#pragma endregion

//Update
/*
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
*/

#pragma region Init

void GCore::LoadTextures()
{
	std::vector<std::wstring> format;
	format.emplace_back(L"dds");
	format.emplace_back(L"png");
	std::vector<std::wstring> files = std::move(GetAllFilesInFolder(L"Content", true, format));
	
	std::unique_ptr<GRiTextureLoader> textureLoader(pRendererFactory->CreateTextureLoader());

	for (auto file : files)
	{
		GRiTexture* tex = textureLoader->LoadTexture(WorkDirectory, file, mTextures.size());
		std::wstring texName = tex->UniqueFileName;
		//mTextures[texName] = std::make_unique<GRiTexture>(tex);
		std::unique_ptr<GRiTexture> temp(tex);
		mTextures[texName] = std::move(temp);
		//mRenderer->pTextures[texName] = mTextures[texName].get();
	}

	LoadSkyTexture(L"Content\\Textures\\Cubemap_LancellottiChapel.dds");

	/*
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
	*/
}

void GCore::LoadMaterials()
{
	int index = 0;

	auto defaultMat = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	defaultMat->UniqueName = L"default";
	defaultMat->Name = L"default";
	defaultMat->MatIndex = index++;
	mTextures[L"Content\\Textures\\sphere_1_BaseColor.png"]->bSrgb = true;
	defaultMat->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_1_BaseColor.png"].get());
	defaultMat->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_1_Normal.png"].get());
	defaultMat->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_1_OcclusionRoughnessMetallic.png"].get());
	mMaterials[L"default"] = std::move(defaultMat);

	auto debug_albedo = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	debug_albedo->UniqueName = L"debug_albedo";
	debug_albedo->Name = L"debug_albedo";
	debug_albedo->MatIndex = index++;
	debug_albedo->ScalarParams.push_back(0.01f);//Albedo
	debug_albedo->ScalarParams.push_back(0.01f);//RGB
	mMaterials[L"debug_albedo"] = std::move(debug_albedo);

	auto debug_normal = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	debug_normal->UniqueName = L"debug_normal";
	debug_normal->Name = L"debug_normal";
	debug_normal->MatIndex = index++;
	debug_normal->ScalarParams.push_back(1.01f);//Normal
	debug_normal->ScalarParams.push_back(0.01f);//RGB
	mMaterials[L"debug_normal"] = std::move(debug_normal);

	auto debug_worldpos = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	debug_worldpos->UniqueName = L"debug_worldpos";
	debug_worldpos->Name = L"debug_worldpos";
	debug_worldpos->MatIndex = index++;
	debug_worldpos->ScalarParams.push_back(2.01f);//WorldPos
	debug_worldpos->ScalarParams.push_back(0.01f);//RGB
	mMaterials[L"debug_worldpos"] = std::move(debug_worldpos);

	auto debug_roughness = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	debug_roughness->UniqueName = L"debug_roughness";
	debug_roughness->Name = L"debug_roughness";
	debug_roughness->MatIndex = index++;
	debug_roughness->ScalarParams.push_back(3.01f);//OcclusionRoughnessMetallic
	debug_roughness->ScalarParams.push_back(2.01f);//Green
	mMaterials[L"debug_roughness"] = std::move(debug_roughness);

	auto debug_metallic = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	debug_metallic->UniqueName = L"debug_metallic";
	debug_metallic->Name = L"debug_metallic";
	debug_metallic->MatIndex = index++;
	debug_metallic->ScalarParams.push_back(3.01f);//OcclusionRoughnessMetallic
	debug_metallic->ScalarParams.push_back(3.01f);//Blue
	mMaterials[L"debug_metallic"] = std::move(debug_metallic);

	auto sphere_1 = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	sphere_1->UniqueName = L"sphere_1";
	sphere_1->Name = L"sphere_1";
	sphere_1->MatIndex = index++;
	mTextures[L"Content\\Textures\\sphere_1_BaseColor.png"]->bSrgb = true;
	sphere_1->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_1_BaseColor.png"].get());
	sphere_1->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_1_Normal.png"].get());
	sphere_1->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_1_OcclusionRoughnessMetallic.png"].get());
	mMaterials[L"sphere_1"] = std::move(sphere_1);

	auto sphere_2 = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	sphere_2->UniqueName = L"sphere_2";
	sphere_2->Name = L"sphere_2";
	sphere_2->MatIndex = index++;
	mTextures[L"Content\\Textures\\sphere_2_BaseColor.png"]->bSrgb = true;
	sphere_2->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_2_BaseColor.png"].get());
	sphere_2->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_2_Normal.png"].get());
	sphere_2->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_2_OcclusionRoughnessMetallic.png"].get());
	mMaterials[L"sphere_2"] = std::move(sphere_2);

	auto sky = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	sky->UniqueName = L"sky";
	sky->Name = L"sky";
	sky->MatIndex = index++;
	//sky->DiffuseSrvHeapIndex = 6;
	//sky->NormalSrvHeapIndex = 7;
	sky->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_1_BaseColor.png"].get());//Diffuse
	sky->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_1_BaseColor.png"].get());//Normal
	//sky->VectorParams.push_back(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));//DiffuseAlbedo
	//sky->VectorParams.push_back(XMFLOAT4(0.1f, 0.1f, 0.1f, 0.f));//FresnelR0
	//sky->ScalarParams.push_back(1.0f);//Roughness
	mMaterials[L"sky"] = std::move(sky);

	auto greasyPanMat = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	greasyPanMat->UniqueName = L"GreasyPan";
	greasyPanMat->Name = L"GreasyPan";
	greasyPanMat->MatIndex = index++;
	mTextures[L"Content\\Textures\\Greasy_Pan_Albedo.png"]->bSrgb = true;
	greasyPanMat->pTextures.push_back(mTextures[L"Content\\Textures\\Greasy_Pan_Albedo.png"].get());
	greasyPanMat->pTextures.push_back(mTextures[L"Content\\Textures\\Greasy_Pan_Normal.png"].get());
	greasyPanMat->pTextures.push_back(mTextures[L"Content\\Textures\\Greasy_Pan_Orm.png"].get());
	mMaterials[L"GreasyPan"] = std::move(greasyPanMat);

	auto rustedIronMat = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	rustedIronMat->UniqueName = L"RustedIron";
	rustedIronMat->Name = L"RustedIron";
	rustedIronMat->MatIndex = index++;
	mTextures[L"Content\\Textures\\Rusted_Iron_Albedo.png"]->bSrgb = true;
	rustedIronMat->pTextures.push_back(mTextures[L"Content\\Textures\\Rusted_Iron_Albedo.png"].get());
	rustedIronMat->pTextures.push_back(mTextures[L"Content\\Textures\\Rusted_Iron_Normal.png"].get());
	rustedIronMat->pTextures.push_back(mTextures[L"Content\\Textures\\Rusted_Iron_Orm.png"].get());
	mMaterials[L"RustedIron"] = std::move(rustedIronMat);

	auto cerberusMat = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	cerberusMat->UniqueName = L"Cerberus";
	cerberusMat->Name = L"Cerberus";
	cerberusMat->MatIndex = index++;
	mTextures[L"Content\\Textures\\Cerberus_Albedo.png"]->bSrgb = true;
	cerberusMat->pTextures.push_back(mTextures[L"Content\\Textures\\Cerberus_Albedo.png"].get());
	cerberusMat->pTextures.push_back(mTextures[L"Content\\Textures\\Cerberus_Normal.png"].get());
	cerberusMat->pTextures.push_back(mTextures[L"Content\\Textures\\Cerberus_Orm.png"].get());
	mMaterials[L"Cerberus"] = std::move(cerberusMat);

	auto fireplaceMat = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	fireplaceMat->UniqueName = L"Fireplace";
	fireplaceMat->Name = L"Fireplace";
	fireplaceMat->MatIndex = index++;
	mTextures[L"Content\\Textures\\Fireplace_Albedo.png"]->bSrgb = true;
	fireplaceMat->pTextures.push_back(mTextures[L"Content\\Textures\\Fireplace_Albedo.png"].get());
	fireplaceMat->pTextures.push_back(mTextures[L"Content\\Textures\\Fireplace_Normal.png"].get());
	fireplaceMat->pTextures.push_back(mTextures[L"Content\\Textures\\Fireplace_Orm.png"].get());
	mMaterials[L"Fireplace"] = std::move(fireplaceMat);
}

void GCore::LoadSkyTexture(std::wstring path)
{
	std::unique_ptr<GRiTextureLoader> textureLoader(pRendererFactory->CreateTextureLoader());

	GRiTexture* tex = textureLoader->LoadTexture(WorkDirectory, path, mTextures.size());
	std::wstring texName = L"skyCubeMap";
	//mTextures[texName].reset(tex);
	std::unique_ptr<GRiTexture> temp(tex);
	mTextures[texName] = std::move(temp);
	//mRenderer->pTextures[texName] = mTextures[texName].get();
}

void GCore::SetWorkDirectory()
{
	TCHAR exeFullPath[MAX_PATH];
	memset(exeFullPath, 0, MAX_PATH);

	GetModuleFileName(NULL, exeFullPath, MAX_PATH);
	WCHAR *p = wcsrchr(exeFullPath, '\\');
	*p = 0x00;

	WorkDirectory = std::wstring(exeFullPath);
	WorkDirectory += L"\\";
}

#pragma endregion

#pragma region Util

std::vector<std::wstring> GCore::GetAllFilesInFolder(std::wstring relPath, bool bCheckFormat, std::vector<std::wstring> format)
{
	std::vector<std::wstring> files;
	intptr_t hFile = 0;
	struct _wfinddata_t fileinfo;
	std::wstring fullPath = WorkDirectory + relPath;
	//relPath = WorkDirectory + relPath;
	std::wstring p;
	hFile = _wfindfirst(p.assign(fullPath).append(L"\\*").c_str(), &fileinfo);
	if (hFile != -1)
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
					std::wstring lFileName = sFileName;
					std::transform(lFileName.begin(), lFileName.end(), lFileName.begin(), ::tolower);
					for (auto f : format)
					{
						if (lFileName.find(L"." + f) == (lFileName.length() - f.length() - 1))
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
	intptr_t hFile = 0;
	struct _wfinddata_t fileinfo;
	std::wstring fullPath = WorkDirectory + relPath;
	//relPath = WorkDirectory + relPath;
	std::wstring p;
	if ((hFile = _wfindfirst(p.assign(fullPath).append(L"\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			if (!(fileinfo.attrib &  _A_SUBDIR))
			{
				if (bCheckFormat)
				{
					bool isOfFormat = false;
					std::wstring sFileName(fileinfo.name);
					std::wstring lFileName = sFileName;
					std::transform(lFileName.begin(), lFileName.end(), lFileName.begin(), ::tolower);
					for (auto f : format)
					{
						if (lFileName.find(L"." + f) == (lFileName.length() - f.length() - 1))
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

//export
/*
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
*/