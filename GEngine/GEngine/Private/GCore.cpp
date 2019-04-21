#pragma once
#include "stdafx.h"
#include "GCore.h"
#include <WindowsX.h>
#include <io.h>
#include "GDxFloat4x4.h"



#pragma region Class

GCore::GCore()
{
	mTimer = std::make_unique<GGiGameTimer>();
	mRenderer = &GDxRenderer::GetRenderer();
	mRenderer->SetTimer(mTimer.get());
	mProject = new GProject();
}

GCore::~GCore()
{

}

GCore& GCore::GetCore()
{
	static GCore *instance = new GCore();
	return *instance;
}

#pragma endregion

#pragma region Main

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
	catch (std::exception& e)
	{
		std::string msg(e.what());
		MessageBox(nullptr, GGiEngineUtil::StringToWString(msg).c_str(), L"Other Exception", MB_OK);
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
			try
			{

				mRenderer->PreInitialize(OutputWindow, width, height);

				LoadProject();

				pRendererFactory = mRenderer->GetFactory();
				//SetWorkDirectory();
				LoadTextures();
				mRenderer->SyncTextures(mTextures);
				LoadMaterials();
				mRenderer->SyncMaterials(mMaterials);
				LoadMeshes();
				mRenderer->SyncMeshes(mMeshes);
				LoadSceneObjects();
				mRenderer->SyncSceneObjects(mSceneObjects, mSceneObjectLayer);
				LoadCameras();
				std::vector<GRiCamera*> cam = {
					mCamera.get(),
					mCubemapSampleCamera[0].get(),
					mCubemapSampleCamera[1].get(),
					mCubemapSampleCamera[2].get(),
					mCubemapSampleCamera[3].get(),
					mCubemapSampleCamera[4].get(),
					mCubemapSampleCamera[5].get()
				};
				mRenderer->SyncCameras(cam);

				mRenderer->Initialize(OutputWindow, width, height);

				SetSceneObjectsCallback();

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
	catch (std::exception& e)
	{
		std::string msg(e.what());
		MessageBox(nullptr, GGiEngineUtil::StringToWString(msg).c_str(), L"Other Exception", MB_OK);
	}
}

void GCore::Update()
{
	OnKeyboardInput(mTimer.get());
	mRenderer->Update(mTimer.get());
}

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
		mTimer->Stop();
		return; 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		mTimer->Start();
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

		return; 0;
	}
}

#pragma endregion

#pragma region Input

void GCore::OnKeyboardInput(const GGiGameTimer* gt)
{
	const float dt = gt->DeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
		mCamera->Walk(150.0f*dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera->Walk(-150.0f*dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera->Strafe(-150.0f*dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera->Strafe(150.0f*dt);

	if (GetAsyncKeyState('E') & 0x8000)
		mCamera->Ascend(150.0f*dt);

	if (GetAsyncKeyState('Q') & 0x8000)
		mCamera->Ascend(-150.0f*dt);

	mCamera->UpdateViewMatrix();
}

void GCore::OnResize()
{
	mCamera->SetLens(0.25f * GGiEngineUtil::PI, mRenderer->AspectRatio(), 1.0f, 1000.0f);
	mRenderer->OnResize();
}

void GCore::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mRenderer->MainWnd());
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
		float dx = 0.25f*static_cast<float>(x - mLastMousePos.x) * GGiEngineUtil::PI / 180.0f;
		float dy = 0.25f*static_cast<float>(y - mLastMousePos.y) * GGiEngineUtil::PI / 180.0f;

		mCamera->Pitch(dy);
		mCamera->RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

#pragma endregion

#pragma region Initialization

void GCore::LoadTextures()
{
	std::vector<std::wstring> format;
	format.emplace_back(L"dds");
	format.emplace_back(L"png");
	std::vector<std::wstring> files = std::move(GetAllFilesInFolder(L"Content", true, format));
	
	std::unique_ptr<GRiTextureLoader> textureLoader(pRendererFactory->CreateTextureLoader());

	for (auto file : files)
	{
		bool bSrgb = false;
		for (auto texInfo : mProject->mTextureInfo)
		{
			if (file == texInfo.UniqueFileName)
			{
				bSrgb = texInfo.bSrgb;
				break;
			}
		}
		GRiTexture* tex = textureLoader->LoadTexture(WorkDirectory, file, bSrgb);
		std::wstring texName = tex->UniqueFileName;
		std::unique_ptr<GRiTexture> temp(tex);
		mTextures[texName] = std::move(temp);
	}

	LoadSkyTexture(L"Content\\Textures\\Cubemap_LancellottiChapel.dds");
}

void GCore::LoadSkyTexture(std::wstring path)
{
	std::unique_ptr<GRiTextureLoader> textureLoader(pRendererFactory->CreateTextureLoader());

	GRiTexture* tex = textureLoader->LoadTexture(WorkDirectory, path, false);
	std::wstring texName = L"skyCubeMap";
	std::unique_ptr<GRiTexture> temp(tex);
	mTextures[texName] = std::move(temp);
}

void GCore::LoadMaterials()
{
	mMaterialIndex = 0;

	auto defaultMat = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	defaultMat->UniqueName = L"default";
	defaultMat->Name = L"default";
	defaultMat->MatIndex = mMaterialIndex++;
	defaultMat->pTextures.push_back(mTextures[L"Content\\Textures\\GE_Default_Albedo.png"].get());
	defaultMat->pTextures.push_back(mTextures[L"Content\\Textures\\GE_Default_Normal.png"].get());
	defaultMat->pTextures.push_back(mTextures[L"Content\\Textures\\GE_Default_Orm.png"].get());
	mMaterials[L"default"] = std::move(defaultMat);

	auto debug_albedo = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	debug_albedo->UniqueName = L"debug_albedo";
	debug_albedo->Name = L"debug_albedo";
	debug_albedo->MatIndex = mMaterialIndex++;
	debug_albedo->ScalarParams.push_back(0.01f);//Albedo
	debug_albedo->ScalarParams.push_back(0.01f);//RGB
	mMaterials[L"debug_albedo"] = std::move(debug_albedo);

	auto debug_normal = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	debug_normal->UniqueName = L"debug_normal";
	debug_normal->Name = L"debug_normal";
	debug_normal->MatIndex = mMaterialIndex++;
	debug_normal->ScalarParams.push_back(1.01f);//Normal
	debug_normal->ScalarParams.push_back(0.01f);//RGB
	mMaterials[L"debug_normal"] = std::move(debug_normal);

	auto debug_worldpos = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	debug_worldpos->UniqueName = L"debug_worldpos";
	debug_worldpos->Name = L"debug_worldpos";
	debug_worldpos->MatIndex = mMaterialIndex++;
	debug_worldpos->ScalarParams.push_back(2.01f);//WorldPos
	debug_worldpos->ScalarParams.push_back(0.01f);//RGB
	mMaterials[L"debug_worldpos"] = std::move(debug_worldpos);

	auto debug_roughness = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	debug_roughness->UniqueName = L"debug_roughness";
	debug_roughness->Name = L"debug_roughness";
	debug_roughness->MatIndex = mMaterialIndex++;
	debug_roughness->ScalarParams.push_back(3.01f);//OcclusionRoughnessMetallic
	debug_roughness->ScalarParams.push_back(2.01f);//Green
	mMaterials[L"debug_roughness"] = std::move(debug_roughness);

	auto debug_metallic = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	debug_metallic->UniqueName = L"debug_metallic";
	debug_metallic->Name = L"debug_metallic";
	debug_metallic->MatIndex = mMaterialIndex++;
	debug_metallic->ScalarParams.push_back(3.01f);//OcclusionRoughnessMetallic
	debug_metallic->ScalarParams.push_back(3.01f);//Blue
	mMaterials[L"debug_metallic"] = std::move(debug_metallic);

	auto sphere_1 = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	sphere_1->UniqueName = L"sphere_1";
	sphere_1->Name = L"sphere_1";
	sphere_1->MatIndex = mMaterialIndex++;
	sphere_1->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_1_BaseColor.png"].get());
	sphere_1->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_1_Normal.png"].get());
	sphere_1->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_1_OcclusionRoughnessMetallic.png"].get());
	mMaterials[L"sphere_1"] = std::move(sphere_1);

	auto sphere_2 = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	sphere_2->UniqueName = L"sphere_2";
	sphere_2->Name = L"sphere_2";
	sphere_2->MatIndex = mMaterialIndex++;
	sphere_2->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_2_BaseColor.png"].get());
	sphere_2->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_2_Normal.png"].get());
	sphere_2->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_2_OcclusionRoughnessMetallic.png"].get());
	mMaterials[L"sphere_2"] = std::move(sphere_2);

	auto sky = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	sky->UniqueName = L"sky";
	sky->Name = L"sky";
	sky->MatIndex = mMaterialIndex++;
	sky->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_1_BaseColor.png"].get());//Diffuse
	sky->pTextures.push_back(mTextures[L"Content\\Textures\\sphere_1_BaseColor.png"].get());//Normal
	mMaterials[L"sky"] = std::move(sky);

	auto greasyPanMat = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	greasyPanMat->UniqueName = L"GreasyPan";
	greasyPanMat->Name = L"GreasyPan";
	greasyPanMat->MatIndex = mMaterialIndex++;
	greasyPanMat->pTextures.push_back(mTextures[L"Content\\Textures\\Greasy_Pan_Albedo.png"].get());
	greasyPanMat->pTextures.push_back(mTextures[L"Content\\Textures\\Greasy_Pan_Normal.png"].get());
	greasyPanMat->pTextures.push_back(mTextures[L"Content\\Textures\\Greasy_Pan_Orm.png"].get());
	mMaterials[L"GreasyPan"] = std::move(greasyPanMat);

	auto rustedIronMat = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	rustedIronMat->UniqueName = L"RustedIron";
	rustedIronMat->Name = L"RustedIron";
	rustedIronMat->MatIndex = mMaterialIndex++;
	rustedIronMat->pTextures.push_back(mTextures[L"Content\\Textures\\Rusted_Iron_Albedo.png"].get());
	rustedIronMat->pTextures.push_back(mTextures[L"Content\\Textures\\Rusted_Iron_Normal.png"].get());
	rustedIronMat->pTextures.push_back(mTextures[L"Content\\Textures\\Rusted_Iron_Orm.png"].get());
	mMaterials[L"RustedIron"] = std::move(rustedIronMat);

	auto cerberusMat = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	cerberusMat->UniqueName = L"Cerberus";
	cerberusMat->Name = L"Cerberus";
	cerberusMat->MatIndex = mMaterialIndex++;
	cerberusMat->pTextures.push_back(mTextures[L"Content\\Textures\\Cerberus_Albedo.png"].get());
	cerberusMat->pTextures.push_back(mTextures[L"Content\\Textures\\Cerberus_Normal.png"].get());
	cerberusMat->pTextures.push_back(mTextures[L"Content\\Textures\\Cerberus_Orm.png"].get());
	mMaterials[L"Cerberus"] = std::move(cerberusMat);

	auto fireplaceMat = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	fireplaceMat->UniqueName = L"Fireplace";
	fireplaceMat->Name = L"Fireplace";
	fireplaceMat->MatIndex = mMaterialIndex++;
	fireplaceMat->pTextures.push_back(mTextures[L"Content\\Textures\\Fireplace_Albedo.png"].get());
	fireplaceMat->pTextures.push_back(mTextures[L"Content\\Textures\\Fireplace_Normal.png"].get());
	fireplaceMat->pTextures.push_back(mTextures[L"Content\\Textures\\Fireplace_Orm.png"].get());
	mMaterials[L"Fireplace"] = std::move(fireplaceMat);

	// Load materials from file.
	{
		std::vector<std::wstring> format;
		format.emplace_back(L"gmat");
		std::vector<std::wstring> files = std::move(GetAllFilesInFolder(L"Content", true, format));

		for (auto file : files)
		{
			auto newMat = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());

			auto matFile = std::make_unique<GMaterial>(newMat.get());
			matFile->UniqueName = file;
			matFile->LoadMaterial(WorkDirectory);
			mMaterialFiles[file] = std::move(matFile);

			newMat->UniqueName = file;
			newMat->Name = GGiEngineUtil::GetFileName(file);
			newMat->MatIndex = mMaterialIndex++;

			for (auto txtName : mMaterialFiles[file]->TextureNames)
			{
				if (mTextures.find(txtName) != mTextures.end())
				{
					newMat->pTextures.push_back(mTextures[txtName].get());
				}
				else
				{
					newMat->pTextures.push_back(mTextures[L"Content\\Textures\\GE_Default_Albedo.png"].get());
				}
			}

			for (auto scalar : mMaterialFiles[file]->ScalarParams)
			{
				newMat->ScalarParams.push_back(scalar);
			}

			std::list<float>::iterator iter = mMaterialFiles[file]->VectorParams.begin();
			for (auto i = 0u; i < (UINT)(mMaterialFiles[file]->VectorParams.size() / 4); i++)
			{
				GGiFloat4* vec = pRendererFactory->CreateFloat4();
				float x = *iter;
				iter++;
				float y = *iter;
				iter++;
				float z = *iter;
				iter++;
				float w = *iter;
				iter++;
				vec->SetElement(0, x);
				vec->SetElement(1, y);
				vec->SetElement(2, z);
				vec->SetElement(3, w);
				newMat->VectorParams.push_back(*vec);
			}

			mMaterialFiles[file]->LoadMaterialData();

			mMaterials[file] = std::move(newMat);
		}
	}
}

void GCore::LoadMeshes()
{

	GRiGeometryGenerator* geoGen = pRendererFactory->CreateGeometryGenerator();

	std::vector<GRiMeshData> meshData;
	GRiMeshData boxMeshData = geoGen->CreateBox(1.0f, 1.0f, 1.0f, 3);
	meshData.push_back(boxMeshData);
	auto geo = pRendererFactory->CreateMesh(meshData);
	geo->UniqueName = L"Box";
	geo->Name = L"Box";
	std::unique_ptr<GRiMesh> temp1(geo);
	mMeshes[geo->UniqueName] = std::move(temp1);

	meshData.clear();
	GRiMeshData gridMeshData = geoGen->CreateGrid(20.0f, 30.0f, 60, 40);
	meshData.push_back(gridMeshData);
	geo = pRendererFactory->CreateMesh(meshData);
	geo->UniqueName = L"Grid";
	geo->Name = L"Grid";
	std::unique_ptr<GRiMesh> temp2(geo);
	mMeshes[geo->UniqueName] = std::move(temp2);

	meshData.clear();
	GRiMeshData sphereMeshData = geoGen->CreateSphere(0.5f, 20, 20);
	meshData.push_back(sphereMeshData);
	geo = pRendererFactory->CreateMesh(meshData);
	geo->UniqueName = L"Sphere";
	geo->Name = L"Sphere";
	std::unique_ptr<GRiMesh> temp3(geo);
	mMeshes[geo->UniqueName] = std::move(temp3);

	meshData.clear();
	GRiMeshData cylinderMeshData = geoGen->CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
	meshData.push_back(cylinderMeshData);
	geo = pRendererFactory->CreateMesh(meshData);
	geo->UniqueName = L"Cylinder";
	geo->Name = L"Cylinder";
	std::unique_ptr<GRiMesh> temp4(geo);
	mMeshes[geo->UniqueName] = std::move(temp4);

	meshData.clear();
	GRiMeshData quadMeshData = geoGen->CreateQuad(0.0f, 1.0f, 1.0f, 1.0f, 0.0f);
	meshData.push_back(quadMeshData);
	geo = pRendererFactory->CreateMesh(meshData);
	geo->UniqueName = L"Quad";
	geo->Name = L"Quad";
	std::unique_ptr<GRiMesh> temp5(geo);
	mMeshes[geo->UniqueName] = std::move(temp5);

	std::vector<std::wstring> format;
	format.emplace_back(L"fbx");
	std::vector<std::wstring> files = std::move(GetAllFilesInFolder(L"Content", true, format));
	for (auto file : files)
	{
		meshData.clear();
		mRenderer->GetFilmboxManager()->ImportFbxFile_Mesh(WorkDirectory + file, meshData);
		geo = pRendererFactory->CreateMesh(meshData);
		geo->UniqueName = file;
		geo->Name = GGiEngineUtil::GetFileName(file);
		std::unique_ptr<GRiMesh> temp(geo);
		mMeshes[geo->UniqueName] = std::move(temp);
	}
}

void GCore::LoadSceneObjects()
{
	UINT index = 0;

	// Create screen quads for light pass and post process.
	std::unique_ptr<GRiSceneObject> fullScreenQuadSO(pRendererFactory->CreateSceneObject());
	fullScreenQuadSO->UniqueName = L"FullScreenQuad";
	fullScreenQuadSO->TexTransform = pRendererFactory->CreateFloat4x4();
	fullScreenQuadSO->ObjIndex = index;
	fullScreenQuadSO->Mat = mMaterials[L"default"].get();
	fullScreenQuadSO->Mesh = mMeshes[L"Quad"].get();
	mSceneObjectLayer[(int)RenderLayer::ScreenQuad].push_back(fullScreenQuadSO.get());
	mSceneObjects[fullScreenQuadSO->UniqueName] = std::move(fullScreenQuadSO);
	index++;

	std::unique_ptr<GRiSceneObject> skySO(pRendererFactory->CreateSceneObject());
	skySO->UniqueName = L"Sky";
	skySO->SetScale(5000.f, 5000.f, 5000.f);
	skySO->ObjIndex = index;
	skySO->TexTransform = pRendererFactory->CreateFloat4x4();
	skySO->Mat = mMaterials[L"sky"].get();
	skySO->Mesh = mMeshes[L"Sphere"].get();
	mSceneObjectLayer[(int)RenderLayer::Sky].push_back(skySO.get());
	mSceneObjects[skySO->UniqueName] = std::move(skySO);
	index++;

	// Create debug quads.
	{
		std::unique_ptr<GRiSceneObject> albedoQuadSO(pRendererFactory->CreateSceneObject());
		albedoQuadSO->UniqueName = L"AlbedoQuad";
		albedoQuadSO->SetScale(.2f, .2f, .2f);
		albedoQuadSO->SetLocation(0.f, 0.f, 0.f);
		albedoQuadSO->TexTransform = pRendererFactory->CreateFloat4x4();
		albedoQuadSO->ObjIndex = index;
		albedoQuadSO->Mat = mMaterials[L"debug_albedo"].get();
		albedoQuadSO->Mesh = mMeshes[L"Quad"].get();
		mSceneObjectLayer[(int)RenderLayer::Debug].push_back(albedoQuadSO.get());
		mSceneObjects[albedoQuadSO->UniqueName] = std::move(albedoQuadSO);
		index++;

		std::unique_ptr<GRiSceneObject> normalQuadSO(pRendererFactory->CreateSceneObject());
		normalQuadSO->UniqueName = L"NormalQuad";
		normalQuadSO->SetScale(.2f, .2f, .2f);
		normalQuadSO->SetLocation(.2f, 0.f, 0.f);
		normalQuadSO->TexTransform = pRendererFactory->CreateFloat4x4();
		normalQuadSO->ObjIndex = index;
		normalQuadSO->Mat = mMaterials[L"debug_normal"].get();
		normalQuadSO->Mesh = mMeshes[L"Quad"].get();
		mSceneObjectLayer[(int)RenderLayer::Debug].push_back(normalQuadSO.get());
		mSceneObjects[normalQuadSO->UniqueName] = std::move(normalQuadSO);
		index++;

		std::unique_ptr<GRiSceneObject> worldPosQuadSO(pRendererFactory->CreateSceneObject());
		worldPosQuadSO->UniqueName = L"WorldPosQuad";
		worldPosQuadSO->SetScale(.2f, .2f, .2f);
		worldPosQuadSO->SetLocation(.4f, 0.f, 0.f);
		worldPosQuadSO->TexTransform = pRendererFactory->CreateFloat4x4();
		worldPosQuadSO->ObjIndex = index;
		worldPosQuadSO->Mat = mMaterials[L"debug_worldpos"].get();
		worldPosQuadSO->Mesh = mMeshes[L"Quad"].get();
		mSceneObjectLayer[(int)RenderLayer::Debug].push_back(worldPosQuadSO.get());
		mSceneObjects[worldPosQuadSO->UniqueName] = std::move(worldPosQuadSO);
		index++;

		std::unique_ptr<GRiSceneObject> roughnessQuadSO(pRendererFactory->CreateSceneObject());
		roughnessQuadSO->UniqueName = L"RoughnessQuad";
		roughnessQuadSO->SetScale(.2f, .2f, .2f);
		roughnessQuadSO->SetLocation(.6f, 0.f, 0.f);
		roughnessQuadSO->TexTransform = pRendererFactory->CreateFloat4x4();
		roughnessQuadSO->ObjIndex = index;
		roughnessQuadSO->Mat = mMaterials[L"debug_roughness"].get();
		roughnessQuadSO->Mesh = mMeshes[L"Quad"].get();
		mSceneObjectLayer[(int)RenderLayer::Debug].push_back(roughnessQuadSO.get());
		mSceneObjects[roughnessQuadSO->UniqueName] = std::move(roughnessQuadSO);
		index++;

		std::unique_ptr<GRiSceneObject> metallicQuadSO(pRendererFactory->CreateSceneObject());
		metallicQuadSO->UniqueName = L"MetallicQuad";
		metallicQuadSO->SetScale(.2f, .2f, .2f);
		metallicQuadSO->SetLocation(.8f, 0.f, 0.f);
		metallicQuadSO->TexTransform = pRendererFactory->CreateFloat4x4();
		metallicQuadSO->ObjIndex = index;
		metallicQuadSO->Mat = mMaterials[L"debug_metallic"].get();
		metallicQuadSO->Mesh = mMeshes[L"Quad"].get();
		mSceneObjectLayer[(int)RenderLayer::Debug].push_back(metallicQuadSO.get());
		mSceneObjects[metallicQuadSO->UniqueName] = std::move(metallicQuadSO);
		index++;
	}

	std::unique_ptr<GRiSceneObject> cerberusSO(pRendererFactory->CreateSceneObject());
	cerberusSO->UniqueName = L"Cerberus";
	cerberusSO->TexTransform = pRendererFactory->CreateFloat4x4();
	cerberusSO->ObjIndex = index;
	cerberusSO->Mat = mMaterials[L"Cerberus"].get();
	cerberusSO->Mesh = mMeshes[L"Content\\Models\\Cerberus.fbx"].get();
	mSceneObjectLayer[(int)RenderLayer::Deferred].push_back(cerberusSO.get());
	mSceneObjects[cerberusSO->UniqueName] = std::move(cerberusSO);
	index++;

	std::unique_ptr<GRiSceneObject> sphereSO_1(pRendererFactory->CreateSceneObject());
	sphereSO_1->SetScale(20.f, 20.f, 20.f);
	sphereSO_1->SetLocation(0.f, -100.f, 0.f);
	sphereSO_1->UniqueName = L"Sphere";
	sphereSO_1->TexTransform = pRendererFactory->CreateFloat4x4();
	sphereSO_1->ObjIndex = index;
	sphereSO_1->Mat = mMaterials[L"default"].get();
	sphereSO_1->Mesh = mMeshes[L"Sphere"].get();
	mSceneObjectLayer[(int)RenderLayer::Deferred].push_back(sphereSO_1.get());
	mSceneObjects[sphereSO_1->UniqueName] = std::move(sphereSO_1);
	index++;

	std::unique_ptr<GRiSceneObject> sphereSO_2(pRendererFactory->CreateSceneObject());
	sphereSO_2->SetScale(20.f, 20.f, 20.f);
	sphereSO_2->SetLocation(20.f, -100.f, 0.f);
	sphereSO_2->UniqueName = L"Sphere_2";
	sphereSO_2->TexTransform = pRendererFactory->CreateFloat4x4();
	sphereSO_2->ObjIndex = index;
	sphereSO_2->Mat = mMaterials[L"GreasyPan"].get();
	sphereSO_2->Mesh = mMeshes[L"Sphere"].get();
	mSceneObjectLayer[(int)RenderLayer::Deferred].push_back(sphereSO_2.get());
	mSceneObjects[sphereSO_2->UniqueName] = std::move(sphereSO_2);
	index++;
}

void GCore::LoadCameras()
{
	mCamera = std::make_unique<GRiCamera>();
	mCamera->SetRendererFactory(pRendererFactory);
	mCamera->SetPosition(0.0f, 2.0f, -5.0f);
	mCamera->SetLens(0.25f * GGiEngineUtil::PI, mRenderer->AspectRatio(), 1.0f, 1000.0f);

	// Build cubemap sampler cameras.
	std::vector<float> center = { 0.0f, 0.0f, 0.0f };
	std::vector<float> worldUp = { 0.0f, 1.0f, 0.0f };

	// Look along each coordinate axis. 
	std::vector<float> targets[6] = {
		{1.0f, 0.0f, 0.0f}, // +X 
		{-1.0f, 0.0f, 0.0f}, // -X 
		{0.0f, 1.0f, 0.0f}, // +Y 
		{0.0f, -1.0f, 0.0f}, // -Y 
		{0.0f, 0.0f, 1.0f}, // +Z 
		{0.0f, 0.0f, -1.0f} // -Z 
	};

	// Use world up vector (0,1,0) for all directions except +Y/-Y.  In these cases, we 
	// are looking down +Y or -Y, so we need a different "up" vector. 
	std::vector<float> ups[6] = {
		{0.0f, 1.0f, 0.0f}, // +X 
		{0.0f, 1.0f, 0.0f}, // -X 
		{0.0f, 0.0f, -1.0f}, // +Y 
		{0.0f, 0.0f, +1.0f}, // -Y 
		{0.0f, 1.0f, 0.0f},	// +Z 
		{0.0f, 1.0f, 0.0f}	// -Z 
	};

	for (int i = 0; i < 6; ++i)
	{
		mCubemapSampleCamera[i] = std::make_unique<GRiCamera>();
		mCubemapSampleCamera[i]->SetRendererFactory(pRendererFactory);
		mCubemapSampleCamera[i]->LookAt(center, targets[i], ups[i]);
		mCubemapSampleCamera[i]->SetLens(0.5f * GGiEngineUtil::PI, 1.0f, 0.1f, 1000.0f);
		mCubemapSampleCamera[i]->UpdateViewMatrix();
	}
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

#pragma region Export

int GCore::GetSceneObjectNum()
{
	return (int)(mSceneObjectLayer[(int)RenderLayer::Deferred].size());
}

const wchar_t* GCore::GetSceneObjectName(int index)
{
	return mSceneObjectLayer[(int)RenderLayer::Deferred][index]->UniqueName.c_str();
}

void GCore::SetSetSceneObjectsCallback(VoidFuncPointerType pSetSceneObjectsCallback)
{
	mSetSceneObjectsCallback = pSetSceneObjectsCallback;
}

void GCore::SetSceneObjectsCallback()
{
	mSetSceneObjectsCallback();
}

void GCore::GetSceneObjectTransform(wchar_t* objName, float* trans)
{
	std::wstring sObjectName(objName);

	std::vector<float> loc = mSceneObjects[sObjectName]->GetLocation();
	std::vector<float> rot = mSceneObjects[sObjectName]->GetRotation();
	std::vector<float> scale = mSceneObjects[sObjectName]->GetScale();

	trans[0] = loc[0];
	trans[1] = loc[1];
	trans[2] = loc[2];
	trans[3] = rot[0];
	trans[4] = rot[1];
	trans[5] = rot[2];
	trans[6] = scale[0];
	trans[7] = scale[1];
	trans[8] = scale[2];
}

void GCore::SetSceneObjectTransform(wchar_t* objName, float* trans)
{
	std::wstring sObjectName(objName);

	mSceneObjects[sObjectName]->SetLocation(trans[0], trans[1], trans[2]);
	mSceneObjects[sObjectName]->SetRotation(trans[3], trans[4], trans[5]);
	mSceneObjects[sObjectName]->SetScale(trans[6], trans[7], trans[8]);
}

bool GCore::GetTextureSrgb(wchar_t* txtName)
{
	std::wstring textureName(txtName);
	return mTextures[textureName]->bSrgb;
}

void GCore::SetTextureSrgb(wchar_t* txtName, bool bSrgb)
{
	std::wstring textureName(txtName);
	mTextures[textureName]->bSrgb = bSrgb;
	std::unique_ptr<GRiTextureLoader> textureLoader(pRendererFactory->CreateTextureLoader());
	GRiTexture* tex = textureLoader->LoadTexture(WorkDirectory, textureName, bSrgb);
	tex->texIndex = mTextures[textureName]->texIndex;
	mTextures[textureName].reset(tex);
	mRenderer->RegisterTexture(mTextures[textureName].get());
}

void GCore::SetWorkDirectory(wchar_t* dir)
{
	std::wstring path(dir);
	WorkDirectory = path;
	/*
	TCHAR exeFullPath[MAX_PATH];
	memset(exeFullPath, 0, MAX_PATH);

	GetModuleFileName(NULL, exeFullPath, MAX_PATH);
	WCHAR *p = wcsrchr(exeFullPath, '\\');
	*p = 0x00;

	WorkDirectory = std::wstring(exeFullPath);
	WorkDirectory += L"\\";
	*/
}

void GCore::SetProjectName(wchar_t* projName)
{
	std::wstring pjName(projName);
	ProjectName = pjName;
}

void GCore::SaveProject()
{
	std::unordered_map<std::wstring, std::unique_ptr<GMaterial>>::iterator it;
	for (it = mMaterialFiles.begin(); it != mMaterialFiles.end(); it++)
	{
		(*it).second->SaveMaterial(WorkDirectory);
	}
	mProject->SaveProject(WorkDirectory + ProjectName + L".gproj", mTextures);
}

void GCore::LoadProject()
{
	mProject->LoadProject(WorkDirectory + ProjectName + L".gproj");
}

void GCore::CreateMaterial(wchar_t* cUniqueName)
{
	std::wstring UniqueName(cUniqueName);
	auto newMat = std::make_unique<GRiMaterial>(*pRendererFactory->CreateMaterial());
	newMat->UniqueName = UniqueName;
	newMat->Name = GGiEngineUtil::GetFileName(UniqueName);
	newMat->MatIndex = mMaterialIndex++;
	newMat->pTextures.push_back(mTextures[L"Content\\Textures\\GE_DefaultTexture_Albedo.png"].get());
	newMat->pTextures.push_back(mTextures[L"Content\\Textures\\GE_DefaultTexture_Normal.png"].get());
	newMat->pTextures.push_back(mTextures[L"Content\\Textures\\GE_DefaultTexture_Orm.png"].get());
	mMaterials[UniqueName] = std::move(newMat);
	auto matFile = std::make_unique<GMaterial>(mMaterials[UniqueName].get());
	matFile->SaveMaterial(WorkDirectory);
	mMaterialFiles[UniqueName] = std::move(matFile);
}

#pragma endregion
