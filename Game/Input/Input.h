#pragma once

#include <SurfMath.h>
#include <cstdint>

enum class KeyCode_e : uint32_t
{
	_CTRL, _SHIFT, _ALT, _ESC,
	_A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z,
	_1, _2, _3, _4, _5, _6, _7, _8, _9, _0,
	MAX
};

namespace Input
{
	bool IsKeyDown(KeyCode_e Key);
	bool IsMouseButtonDown(int Button);
	float2 GetMouseDelta();

	int Win_InputHandler(void* WindowHandle, uint32_t Message, uint64_t wParam, int64_t lParam);
}