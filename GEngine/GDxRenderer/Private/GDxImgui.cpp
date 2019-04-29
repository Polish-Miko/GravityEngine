#include "stdafx.h"
#include "GDxImgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"


GDxImgui::GDxImgui()
{
}


GDxImgui::~GDxImgui()
{
}

void GDxImgui::Initialize(HWND handleWnd, ID3D12Device* pDevice, int NumFrameResources, ID3D12DescriptorHeap* pDescriptorHeap)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(handleWnd);
	ImGui_ImplDX12_Init(pDevice, NumFrameResources,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		pDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'misc/fonts/README.txt' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);
}

void GDxImgui::BeginFrame()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
}

void GDxImgui::SetGUIContent(bool bShowGizmo, const float *cameraView, float *cameraProjection, float* objectLocation, float* objectRotation, float* objectScale)
{
	static bool show_demo_window = false;

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	{
		ImGui::Begin("Statistics");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	{
		ImGui::Begin("Manipulation");
		EditTransform(bShowGizmo, cameraView, cameraProjection, objectLocation, objectRotation, objectScale);
		ImGui::End();
	}
}

void GDxImgui::Render(ID3D12GraphicsCommandList* cmdList)
{
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void GDxImgui::ShutDown()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void GDxImgui::EditTransform(bool bShowGizmo, const float *cameraView, float *cameraProjection, float* objectLocation, float* objectRotation, float* objectScale)
{
	static bool bSelectMode = false;
	static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
	static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::LOCAL);
	static bool useSnap = false;
	static float snap[3] = { 1.f, 1.f, 1.f };

	if (ImGui::IsKeyPressed(81) && !ImGui::IsMouseDown(1)) // Q Key
	{
		bSelectMode = true;
	}
	if (ImGui::IsKeyPressed(87) && !ImGui::IsMouseDown(1)) // W Key
	{
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
		bSelectMode = false;
	}
	if (ImGui::IsKeyPressed(69) && !ImGui::IsMouseDown(1)) // E Key
	{
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
		bSelectMode = false;
	}
	if (ImGui::IsKeyPressed(82) && !ImGui::IsMouseDown(1)) // R Key
	{
		mCurrentGizmoOperation = ImGuizmo::SCALE;
		bSelectMode = false;
	}
	if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
		mCurrentGizmoOperation = ImGuizmo::SCALE;

	if (mCurrentGizmoOperation != ImGuizmo::SCALE)
	{
		if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
			mCurrentGizmoMode = ImGuizmo::LOCAL;
		ImGui::SameLine();
		if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
			mCurrentGizmoMode = ImGuizmo::WORLD;
	}
	if (ImGui::IsKeyPressed(83))
		useSnap = !useSnap;
	ImGui::Checkbox("", &useSnap);
	ImGui::SameLine();

	switch (mCurrentGizmoOperation)
	{
	case ImGuizmo::TRANSLATE:
		ImGui::InputFloat3("Snap", &snap[0]);
		break;
	case ImGuizmo::ROTATE:
		ImGui::InputFloat("Angle Snap", &snap[0]);
		break;
	case ImGuizmo::SCALE:
		ImGui::InputFloat("Scale Snap", &snap[0]);
		break;
	}

	float matrix[16];
	ImGuizmo::RecomposeMatrixFromComponents(objectLocation, objectRotation, objectScale, matrix);

	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	if (!bSelectMode && bShowGizmo)
		ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, useSnap ? &snap[0] : NULL, NULL, NULL);

	ImGuizmo::DecomposeMatrixToComponents(matrix, objectLocation, objectRotation, objectScale);
}


