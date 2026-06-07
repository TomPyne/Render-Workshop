#include "Input.h"

#include <Shared/Logging/Logging.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h> // GET_X_LPARAM(), GET_Y_LPARAM()

// I have been using ImGui for my input so far, this is a step away so that I don't need ImGui running to be able to handle input.
// However this is created by lifting the relevant code from imgui_impl_win32.cpp, so it is still very much based on that and may be changed in the future to be more custom to my needs.

struct InputData
{
	bool KeyStates[(uint32_t)KeyCode_e::MAX] = {0};
	float2 MousePosition = float2(0.0f);
	float2 MouseDelta = float2(0.0f);
	uint8_t MouseButtonStates = 0;
	float MouseWheelDelta = 0.0f;
};
InputData g_InputData;

static KeyCode_e Win_VirtualKeyToKeyCode(WPARAM VirtualKey)
{
	switch (VirtualKey)
	{
	case VK_CONTROL: return KeyCode_e::_CTRL;
	case VK_SHIFT: return KeyCode_e::_SHIFT;
	case VK_MENU: return KeyCode_e::_ALT;
	case VK_ESCAPE: return KeyCode_e::_ESC;
	case 'A': return KeyCode_e::_A;
	case 'B': return KeyCode_e::_B;
	case 'C': return KeyCode_e::_C;
	case 'D': return KeyCode_e::_D;
	case 'E': return KeyCode_e::_E;
	case 'F': return KeyCode_e::_F;
	case 'G': return KeyCode_e::_G;
	case 'H': return KeyCode_e::_H;
	case 'I': return KeyCode_e::_I;
	case 'J': return KeyCode_e::_J;
	case 'K': return KeyCode_e::_K;
	case 'L': return KeyCode_e::_L;
	case 'M': return KeyCode_e::_M;
	case 'N': return KeyCode_e::_N;
	case 'O': return KeyCode_e::_O;
	case 'P': return KeyCode_e::_P;
	case 'Q': return KeyCode_e::_Q;
	case 'R': return KeyCode_e::_R;
	case 'S': return KeyCode_e::_S;
	case 'T': return KeyCode_e::_T;
	case 'U': return KeyCode_e::_U;
	case 'V': return KeyCode_e::_V;
	case 'W': return KeyCode_e::_W;
	case 'X': return KeyCode_e::_X;
	case 'Y': return KeyCode_e::_Y;
	case 'Z': return KeyCode_e::_Z;
	case '1': return KeyCode_e::_1;
	case '2': return KeyCode_e::_2;
	case '3': return KeyCode_e::_3;
	case '4': return KeyCode_e::_4;
	case '5': return KeyCode_e::_5;
	case '6': return KeyCode_e::_6;
	case '7': return KeyCode_e::_7;
	case '8': return KeyCode_e::_8;
	case '9': return KeyCode_e::_9;
	case '0': return KeyCode_e::_0;
	default:
		ASSERT0MSG("Unmapped virtual key: %d", VirtualKey);
		return (KeyCode_e)0; // Invalid key
	}
}

bool Input::IsKeyDown(KeyCode_e Key)
{
	return g_InputData.KeyStates[(uint32_t)Key];
}

bool Input::IsMouseButtonDown(int Button)
{
	return (g_InputData.MouseButtonStates & (1 << Button)) != 0;
}

float2 Input::GetMouseDelta()
{
	return g_InputData.MouseDelta;
}

int Input::Win_InputHandler(void* WindowHandle, uint32_t Message, uint64_t wParam, int64_t lParam)
{
	switch (Message)
	{
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
	{
		POINT MousePos = { (LONG)GET_X_LPARAM(lParam), (LONG)GET_Y_LPARAM(lParam) };

		g_InputData.MouseDelta.x = static_cast<float>(MousePos.x) - g_InputData.MousePosition.x;
		g_InputData.MouseDelta.y = static_cast<float>(MousePos.y) - g_InputData.MousePosition.y;
		g_InputData.MousePosition.x = static_cast<float>(MousePos.x);
		g_InputData.MousePosition.y = static_cast<float>(MousePos.y);

		return 0;
	}
	case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
	{
		int button = 0;
		if (Message == WM_LBUTTONDOWN || Message == WM_LBUTTONDBLCLK) { button = 0; }
		if (Message == WM_RBUTTONDOWN || Message == WM_RBUTTONDBLCLK) { button = 1; }
		if (Message == WM_MBUTTONDOWN || Message == WM_MBUTTONDBLCLK) { button = 2; }
		if (Message == WM_XBUTTONDOWN || Message == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
		g_InputData.MouseButtonStates |= (1 << button);

		return 0;
	}
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
	{
		int button = 0;
		if (Message == WM_LBUTTONUP) { button = 0; }
		if (Message == WM_RBUTTONUP) { button = 1; }
		if (Message == WM_MBUTTONUP) { button = 2; }
		if (Message == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
		g_InputData.MouseButtonStates &= ~(1 << button);

		return 0;
	}
	case WM_MOUSEWHEEL:
		g_InputData.MouseWheelDelta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		return 0;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	{
		const bool is_key_down = (Message == WM_KEYDOWN || Message == WM_SYSKEYDOWN);
		const KeyCode_e Key = Win_VirtualKeyToKeyCode(wParam);

		g_InputData.KeyStates[(uint32_t)Key] = is_key_down;

		return 0;
	}
	}

	return 0;
}
