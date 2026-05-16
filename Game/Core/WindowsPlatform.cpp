#include "WindowsPlatform.h"

#include "GameApp.h"
#include <Shared/Logging/Logging.h>
#include <Windows.h>

GameApp_c* GApp = nullptr;
HWND GMainWindowHandle = nullptr;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void WindowsPlatformMain(const char* WindowTitle, int Width, int Height, GameApp_c* App)
{
	if (!App)
		return;

	GApp = App;

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, WindowTitle, NULL };
	::RegisterClassEx(&wc);
	GMainWindowHandle = ::CreateWindow(wc.lpszClassName, WindowTitle, WS_OVERLAPPEDWINDOW, 100, 100, Width, Height, NULL, NULL, wc.hInstance, NULL);

	WINCHECK(GMainWindowHandle != nullptr);

	if (!GApp->Init())
	{
		GApp->Shutdown();
		::DestroyWindow(GMainWindowHandle);
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return;
	}

	::ShowWindow(GMainWindowHandle, SW_SHOWDEFAULT);
	::UpdateWindow(GMainWindowHandle);

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

		GApp->Update();
	}

	GApp->Shutdown();
	::DestroyWindow(GMainWindowHandle);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);
}

void* GetMainWindowHandle()
{
	return (void*)GMainWindowHandle;
}

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	GApp->HandleWindowsMessage(hWnd, msg, wParam, lParam);

	switch (msg)
	{
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{			
			const int w = (int)LOWORD(lParam);
			const int h = (int)HIWORD(lParam);
			GApp->Resize(w, h);

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