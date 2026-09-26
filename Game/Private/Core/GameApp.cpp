#include "Core/GameApp.h"

#include "Assets/MaterialManager.h"
#include "Core/WindowsPlatform.h"
#include "Input/Input.h"
#include "Object/CameraComponent.h"
#include "Object/DebugCameraObject.h"
#include "Object/FlyControllerComponent.h"
#include "Object/MeshComponent.h"
#include "Object/RuntimeMeshComponent.h"
#include "Rendering/Materials.h"
#include "Rendering/SpaceRenderer.h"
#include "Game/Private/Rendering/Materials/BRDFMaterial.h"
#include "Space/Space.h"
#include "Tools/PerfStats.h"


#include <RenderImGui/imgui/imgui.h>
#include <RenderImGui/imgui/backends/imgui_impl_win32.h>
#include <RenderImGui/Source/Public/imgui_impl_render.h>
#include <Render/Render.h>

GameApp_c* GApp;

bool GameApp_c::Init()
{
	rl::RenderInitParams Params = GetAppRenderParams();	

	if (!rl::Render_Init(Params))
	{
		return false;
	}

	HWND Hwnd = (HWND)GetMainWindowHandle();

	MainRenderView = rl::CreateRenderViewPtr((intptr_t)Hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplWin32_Init(Hwnd);
	ImGui_ImplRender_Init(rl::RenderFormat::R8G8B8A8_UNORM);

	ImGui_ImplRender_NewFrame();

	Clock = {};

	return true;
}

void GameApp_c::Main()
{
	PreUpdate();

	const float DeltaSeconds = Clock.GetDeltaSeconds();
	PerfStats::RecordFrame(DeltaSeconds);

	PerfStats::BeginUpdate();
	Update(DeltaSeconds);
	PerfStats::EndUpdate();

	ImGuiUpdate();

	PerfStats::BeginRender();
	Render();
	PerfStats::EndRender();
}

void GameApp_c::Shutdown()
{
	SpaceRenderer.reset();
	rl::Render_ShutDown();
}

std::shared_ptr<SpaceRenderer_c> GameApp_c::CreateSpaceRenderer() const
{
	return std::make_shared<SpaceRenderer_c>();
}

void GameApp_c::RegisterClasses()
{
	if (!Space)
		return;

	// Objects
	Space->RegisterObjectClass<MeshObject_c>();
	Space->RegisterObjectClass<RuntimeMeshObject_c>();
	Space->RegisterObjectClass<SpatialObject_c>();
	Space->RegisterObjectClass<DebugCameraObject_c>();

	// Components
	Space->RegisterComponentClass<CameraComponent_c>();
	Space->RegisterComponentClass<FlyControllerComponent_c>();
	Space->RegisterComponentClass<MeshComponent_c>();

	// Materials
	MaterialManager::RegisterMaterialShaderClass<DefaultMaterialShader_c>(L"DefaultMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<ErrorMaterialShader_c>(L"ErrorMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<BRDFMaterialShader_c>(L"DefaultBRDFMaterialShader");
}

void GameApp_c::Load()
{
	Space = std::make_shared<Space_c>();
	RegisterClasses();

	SpaceRenderer = CreateSpaceRenderer();
	SpaceRenderer->Init();
}

void GameApp_c::PreUpdate()
{
	Clock.Tick();
	Input::NewFrame();
}

void GameApp_c::Update(float Delta)
{
	if (Space)
	{
		Space->Update(Delta);
	}
}

void GameApp_c::Render()
{
	rl::Render_BeginFrame();

	ImGui_ImplRender_NewFrame();
	ImGui::Render();

	ImRenderFrameData* FrameData = ImGui_ImplRender_PrepareFrameData(ImGui::GetDrawData());

	rl::Render_BeginRenderFrame();

	rl::CommandListSubmissionGroup CLGroup(rl::CommandListType::GRAPHICS);

	rl::CommandList* MainCL = CLGroup.CreateCommandList();

	rl::UploadBuffers(MainCL);

	MainCL->TransitionResource(MainRenderView->GetCurrentBackBufferTexture(), rl::ResourceTransitionState::PRESENT, rl::ResourceTransitionState::RENDER_TARGET);

	MainRenderView->ClearCurrentBackBufferTarget(MainCL);

	if (Space && SpaceRenderer)
	{
		SpaceRendererScreenInfo_s Info = {};
		Info.Width = MainRenderView->Width;
		Info.Height = MainRenderView->Height;
		Info.RenderView = MainRenderView.get();
		SpaceRenderer->RenderSpace(Info, Space.get(), CLGroup);
	}

	rl::CommandList* PostCL = CLGroup.CreateCommandList();

	PostCL->SetRootSignature(ImGui_ImplRender_GetRootSignature());

	rl::RenderTargetView_t BackBufferRtv = MainRenderView->GetCurrentBackBufferRTV();

	PostCL->SetRenderTargets(&BackBufferRtv, 1, rl::DepthStencilView_t::INVALID);

	ImGui_ImplRender_RenderDrawData(FrameData, ImGui::GetDrawData(), PostCL);

	ImGui_ImplRender_ReleaseFrameData(FrameData);

	PostCL->TransitionResource(MainRenderView->GetCurrentBackBufferTexture(), rl::ResourceTransitionState::RENDER_TARGET, rl::ResourceTransitionState::PRESENT);

	CLGroup.Submit();

	rl::Render_EndFrame();

	MainRenderView->Present(true);
}

void GameApp_c::ImGuiUpdate()
{
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();
}

void GameApp_c::Resize(int Width, int Height)
{
	MainRenderView->Resize(Width, Height);
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT GameApp_c::HandleWindowsMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Input::Win_InputHandler((void*)hWnd, msg, wParam, lParam);
	ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
	return 0;
}

uint2 GameApp_c::GetScreenSize() const
{
	if (MainRenderView)
	{
		return uint2(MainRenderView->Width, MainRenderView->Height);
	}
	return uint2(0,0);
}

rl::RenderInitParams GameApp_c::GetAppRenderParams() const
{
	rl::RenderInitParams Params = {};
	Params.DebugEnabled = true;
	return Params;
}
