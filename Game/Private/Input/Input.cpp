#include "Input/Input.h"

#include <Shared/Logging/Logging.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h> // GET_X_LPARAM(), GET_Y_LPARAM()

// I have been using ImGui for my input so far, this is a step away so that I don't need ImGui running to be able to handle input.
// However this is created by lifting the relevant code from imgui_impl_win32.cpp, so it is still very much based on that and may be changed in the future to be more custom to my needs.

enum class KeyState_e : uint8_t
{
	UNPRESSED,
	PRESSED,
	HELD,
};

struct InputData
{
	// Raw physical state, driven by the window messages.
	bool KeyDownRaw[(uint32_t)KeyCode_e::MAX] = {};
	// Sticky down edge, so a press and release that both land between two frames isn't dropped.
	bool KeyPressedRaw[(uint32_t)KeyCode_e::MAX] = {};
	// Per frame snapshot, this is what the game reads.
	KeyState_e KeyStates[(uint32_t)KeyCode_e::MAX] = { KeyState_e::UNPRESSED };
	int2 MousePosition = int2(0);
	int2 MousePrevPosition = int2(0);
	int2 MouseDelta = int2(0);
	uint8_t MouseButtonStates = 0;
	float MouseWheelDelta = 0.0f;
	HWND WindowHandle = nullptr;
	bool MouseCaptured = false;
	POINT RestoreCursorPosition = {};
};
InputData g_InputData;

static bool Win_GetWindowCentre(HWND Window, POINT& OutCentre)
{
	RECT ClientRect;
	if (!Window || !GetClientRect(Window, &ClientRect))
	{
		return false;
	}

	OutCentre.x = (ClientRect.left + ClientRect.right) / 2;
	OutCentre.y = (ClientRect.top + ClientRect.bottom) / 2;

	return ClientToScreen(Window, &OutCentre) != FALSE;
}

static KeyCode_e Win_VirtualKeyToKeyCode(WPARAM VirtualKey)
{
	switch (VirtualKey)
	{
	case VK_CONTROL: return KeyCode_e::_CTRL;
	case VK_SHIFT: return KeyCode_e::_SHIFT;
	case VK_MENU: return KeyCode_e::_ALT;
	case VK_ESCAPE: return KeyCode_e::_ESC;
	case VK_F1: return KeyCode_e::_F1;
	case VK_F2: return KeyCode_e::_F2;
	case VK_F3: return KeyCode_e::_F3;
	case VK_F4: return KeyCode_e::_F4;
	case VK_F5: return KeyCode_e::_F5;
	case VK_F6: return KeyCode_e::_F6;
	case VK_F7: return KeyCode_e::_F7;
	case VK_F8: return KeyCode_e::_F8;
	case VK_F9: return KeyCode_e::_F9;
	case VK_F10: return KeyCode_e::_F10;
	case VK_F11: return KeyCode_e::_F11;
	case VK_F12: return KeyCode_e::_F12;
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
		return KeyCode_e::INVALID;
	}
}

void Input::NewFrame()
{
	for (uint32_t KeyIndex = 0; KeyIndex < (uint32_t)KeyCode_e::MAX; ++KeyIndex)
	{
		if (g_InputData.KeyPressedRaw[KeyIndex])
		{
			g_InputData.KeyStates[KeyIndex] = KeyState_e::PRESSED;
		}
		else if (g_InputData.KeyDownRaw[KeyIndex])
		{
			g_InputData.KeyStates[KeyIndex] = KeyState_e::HELD;
		}
		else
		{
			g_InputData.KeyStates[KeyIndex] = KeyState_e::UNPRESSED;
		}

		g_InputData.KeyPressedRaw[KeyIndex] = false;
	}

	POINT Centre;
	if (g_InputData.MouseCaptured && Win_GetWindowCentre(g_InputData.WindowHandle, Centre))
	{
		POINT Cursor;
		if (GetCursorPos(&Cursor))
		{
			g_InputData.MouseDelta.x = Cursor.x - Centre.x;
			g_InputData.MouseDelta.y = Cursor.y - Centre.y;

			SetCursorPos(Centre.x, Centre.y);
		}
		else
		{
			g_InputData.MouseDelta = float2(0.0f);
		}

		// Keep the uncaptured path's baseline fresh so releasing capture doesn't
		// produce a delta spike from a stale previous position.
		g_InputData.MousePrevPosition = g_InputData.MousePosition;

		return;
	}

	g_InputData.MouseDelta.x = g_InputData.MousePosition.x - g_InputData.MousePrevPosition.x;
	g_InputData.MouseDelta.y = g_InputData.MousePosition.y - g_InputData.MousePrevPosition.y;

	g_InputData.MousePrevPosition = g_InputData.MousePosition;
}

void Input::SetMouseCaptured(bool Captured)
{
	if (Captured == g_InputData.MouseCaptured)
	{
		return;
	}

	POINT Centre;
	if (Captured)
	{
		if (!Win_GetWindowCentre(g_InputData.WindowHandle, Centre))
		{
			return;
		}

		GetCursorPos(&g_InputData.RestoreCursorPosition);
		SetCapture(g_InputData.WindowHandle);
		ShowCursor(FALSE);
		SetCursorPos(Centre.x, Centre.y);
	}
	else
	{
		ReleaseCapture();
		ShowCursor(TRUE);
		SetCursorPos(g_InputData.RestoreCursorPosition.x, g_InputData.RestoreCursorPosition.y);
	}

	g_InputData.MouseDelta = float2(0.0f);
	g_InputData.MouseCaptured = Captured;
}

bool Input::IsMouseCaptured()
{
	return g_InputData.MouseCaptured;
}

bool Input::IsKeyDown(KeyCode_e Key)
{
	return g_InputData.KeyStates[(uint32_t)Key] > KeyState_e::UNPRESSED;
}

bool Input::IsKeyPressed(KeyCode_e Key)
{
	return g_InputData.KeyStates[(uint32_t)Key] == KeyState_e::PRESSED;
}

bool Input::IsMouseButtonDown(int Button)
{
	return (g_InputData.MouseButtonStates & (1 << Button)) != 0;
}

int2 Input::GetMouseDelta()
{
	return g_InputData.MouseDelta;
}

int2 Input::GetMousePosition()
{
	return g_InputData.MousePosition;
}

int Input::Win_InputHandler(void* WindowHandle, uint32_t Message, uint64_t wParam, int64_t lParam)
{
	g_InputData.WindowHandle = static_cast<HWND>(WindowHandle);

	switch (Message)
	{
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
	{
		POINT MousePos = { (LONG)GET_X_LPARAM(lParam), (LONG)GET_Y_LPARAM(lParam) };

		g_InputData.MousePosition.x = static_cast<int32_t>(MousePos.x);
		g_InputData.MousePosition.y = static_cast<int32_t>(MousePos.y);

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
	case WM_KILLFOCUS:
		SetMouseCaptured(false);
		g_InputData.MouseButtonStates = 0;
		memset(g_InputData.KeyDownRaw, 0, sizeof(g_InputData.KeyDownRaw));
		memset(g_InputData.KeyPressedRaw, 0, sizeof(g_InputData.KeyPressedRaw));
		memset(g_InputData.KeyStates, 0, sizeof(g_InputData.KeyStates));
		return 0;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	{
		const bool KeyDown = (Message == WM_KEYDOWN || Message == WM_SYSKEYDOWN);
		const KeyCode_e Key = Win_VirtualKeyToKeyCode(wParam);

		if (Key == KeyCode_e::INVALID)
		{
			return 0;
		}

		if (KeyDown)
		{
			// Windows repeats WM_KEYDOWN while the key is held, only the first one is an edge.
			if (!g_InputData.KeyDownRaw[(uint32_t)Key])
			{
				g_InputData.KeyPressedRaw[(uint32_t)Key] = true;
			}

			g_InputData.KeyDownRaw[(uint32_t)Key] = true;
		}
		else
		{
			g_InputData.KeyDownRaw[(uint32_t)Key] = false;
		}

		return 0;
	}
	}

	return 0;
}
