#include "Core/WindowsPlatform.h"

#include "Core/GameApp.h"
#include <Shared/CommandLine/CommandLine.h>
#include <Shared/Logging/Logging.h>
#include <Windows.h>

HWND GMainWindowHandle = nullptr;

// Popup windows get WM_SIZE during CreateWindow, before the app is ready to resize
static bool bAppInitialised = false;

static CommandLineSwitch_c Windowed("windowed", "Run in a window instead of borderless fullscreen");
static CommandLineValue_c<int> WindowWidth("windowwidth", 0, "Windowed client width, 0 uses the app default");
static CommandLineValue_c<int> WindowHeight("windowheight", 0, "Windowed client height, 0 uses the app default");

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void WindowsPlatformMain(const char* WindowTitle, int Width, int Height)
{
	if (!GApp)
		return;

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, WindowTitle, NULL };
	::RegisterClassEx(&wc);

	if (Windowed)
	{
		RECT WindowRect = { 0, 0, WindowWidth.Get() > 0 ? WindowWidth.Get() : Width, WindowHeight.Get() > 0 ? WindowHeight.Get() : Height };
		::AdjustWindowRect(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE);
		GMainWindowHandle = ::CreateWindow(wc.lpszClassName, WindowTitle, WS_OVERLAPPEDWINDOW, 100, 100, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, NULL, NULL, wc.hInstance, NULL);
	}
	else
	{
		MONITORINFO MonitorInfo = { sizeof(MONITORINFO) };
		::GetMonitorInfo(::MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo);
		const RECT& MonitorRect = MonitorInfo.rcMonitor;
		GMainWindowHandle = ::CreateWindow(wc.lpszClassName, WindowTitle, WS_POPUP, MonitorRect.left, MonitorRect.top, MonitorRect.right - MonitorRect.left, MonitorRect.bottom - MonitorRect.top, NULL, NULL, wc.hInstance, NULL);
	}

	WINCHECK(GMainWindowHandle != nullptr);

	if (!GApp->Init())
	{
		GApp->Shutdown();
		::DestroyWindow(GMainWindowHandle);
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return;
	}

	bAppInitialised = true;

	RECT ClientRect = {};
	::GetClientRect(GMainWindowHandle, &ClientRect);
	GApp->Resize(ClientRect.right - ClientRect.left, ClientRect.bottom - ClientRect.top);

	::ShowWindow(GMainWindowHandle, SW_SHOWDEFAULT);
	::UpdateWindow(GMainWindowHandle);

	GApp->Load();

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

		GApp->Main();
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
		if (wParam != SIZE_MINIMIZED && bAppInitialised)
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