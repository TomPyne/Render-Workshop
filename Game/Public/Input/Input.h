#pragma once

#include <SurfMath.h>
#include <cstdint>

enum class KeyCode_e : uint32_t
{
	INVALID, // Deliberately first, so an unmapped key lands in a dead slot rather than on a real key
	_CTRL, _SHIFT, _ALT, _ESC,
	_A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z,
	_1, _2, _3, _4, _5, _6, _7, _8, _9, _0,
	_F1, _F2, _F3, _F4, _F5, _F6, _F7, _F8, _F9, _F10, _F11, _F12,
	MAX
};

namespace Input
{
	void NewFrame();

	bool IsKeyDown(KeyCode_e Key);
	bool IsKeyPressed(KeyCode_e Key);
	bool IsMouseButtonDown(int Button);
	float2 GetMouseDelta();
	float2 GetMousePosition();

	// While captured the cursor is hidden and recentred every frame, so mouse
	// deltas keep accumulating instead of stopping at the window edge.
	void SetMouseCaptured(bool Captured);
	bool IsMouseCaptured();

	int Win_InputHandler(void* WindowHandle, uint32_t Message, uint64_t wParam, int64_t lParam);
}