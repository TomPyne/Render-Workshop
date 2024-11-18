
#include "imgui.h"
#include "ImGui/imgui_impl_render.h"
#include "backends/imgui_impl_win32.h"
#include <SurfClock.h>
#include <Render/Render.h>
#include "ComputeSeminarApp.h"

using namespace tpr;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
int main()
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "Render Example", NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, "Compute Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	{
		RenderInitParams params = GetAppRenderParams();

		if (!Render_Init(params))
		{
			Render_ShutDown();
			::UnregisterClass(wc.lpszClassName, wc.hInstance);
			return 1;
		}
	}

	RenderViewPtr view = CreateRenderViewPtr((intptr_t)hwnd);

	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplRender_Init(RenderFormat::R8G8B8A8_UNORM);

	SurfClock Clock = {};

	InitializeApp();

	// Main loop
	bool bQuit = false;
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (bQuit == false && msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			continue;
		}

		Clock.Tick();

		const float deltaSeconds = Clock.GetDeltaSeconds();

		Update(deltaSeconds);

		Render_BeginFrame();

		{
			ImGui_ImplRender_NewFrame();

			ImGui_ImplWin32_NewFrame();

			ImGui::NewFrame();

			ImGui::Render();
		}

		Render_BeginRenderFrame();

		CommandListSubmissionGroup clGroup(CommandListType::GRAPHICS);

		CommandList* initialCl = clGroup.CreateCommandList();

		UploadBuffers(initialCl);

		initialCl->TransitionResource(view->GetCurrentBackBufferTexture(), ResourceTransitionState::PRESENT, ResourceTransitionState::RENDER_TARGET);

		Render(view.get(), &clGroup, deltaSeconds);

		CommandList* finalCl = clGroup.CreateCommandList();

		finalCl->TransitionResource(view->GetCurrentBackBufferTexture(), ResourceTransitionState::RENDER_TARGET, ResourceTransitionState::PRESENT);

		clGroup.Submit();

		Render_EndFrame();

		view->Present(true);
	}

	ImGui_ImplRender_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	Render_ShutDown();

	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);
}

// Win32 message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			const int w = (int)LOWORD(lParam);
			const int h = (int)HIWORD(lParam);

			if (RenderView* rv = GetRenderViewForHwnd((intptr_t)hWnd))
			{
				rv->Resize(w, h);
			}

			ResizeApp(w, h);
			return 0;
		}
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
