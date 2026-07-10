#include "GameApp.h"

#include "Game/Input/Input.h"
#include "Game/Rendering/SpaceRenderer.h"
#include "Game/Space/Space.h"
#include "WindowsPlatform.h"

#include <Render/Render.h>

bool GameApp_c::Init()
{
	rl::RenderInitParams Params = GetAppRenderParams();

	Params.DebugEnabled = true;

	if (!rl::Render_Init(Params))
	{
		return false;
	}

	HWND Hwnd = (HWND)GetMainWindowHandle();

	MainRenderView = rl::CreateRenderViewPtr((intptr_t)Hwnd);

	Clock = {};

	//InitializeApp();

	return true;
}

void GameApp_c::Shutdown()
{
	rl::Render_ShutDown();
}

void GameApp_c::Load()
{
	Space = std::make_shared<Space_c>();
	SpaceRenderer = std::make_shared<SpaceRenderer_c>();
}

void GameApp_c::Update()
{
	Clock.Tick();
	const float DeltaSeconds = Clock.GetDeltaSeconds();

	if (Space)
	{
		Space->Update(DeltaSeconds);
	}

	Render();
}

void GameApp_c::Render()
{
	rl::Render_BeginFrame();

	rl::Render_BeginRenderFrame();

	rl::CommandListSubmissionGroup CLGroup(rl::CommandListType::GRAPHICS);

	rl::CommandList* MainCL = CLGroup.CreateCommandList();

	rl::UploadBuffers(MainCL);

	MainCL->TransitionResource(MainRenderView->GetCurrentBackBufferTexture(), rl::ResourceTransitionState::PRESENT, rl::ResourceTransitionState::RENDER_TARGET);

	MainRenderView->ClearCurrentBackBufferTarget(MainCL);

	if (Space && SpaceRenderer)
	{
		SpaceRenderer->RenderSpace(Space.get(), CLGroup);
	}

	MainCL->TransitionResource(MainRenderView->GetCurrentBackBufferTexture(), rl::ResourceTransitionState::RENDER_TARGET, rl::ResourceTransitionState::PRESENT);

	CLGroup.Submit();

	rl::Render_EndFrame();

	MainRenderView->Present(true);
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

rl::RenderInitParams GameApp_c::GetAppRenderParams() const
{
	return rl::RenderInitParams();
}
