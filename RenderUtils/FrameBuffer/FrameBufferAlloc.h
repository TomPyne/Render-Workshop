#pragma once

#include <cstdint>

struct FrameBuffer_s;

struct FrameBufferAlloc_t
{
	FrameBuffer_s* Owner = nullptr;
	uint32_t Offset = 0u;
	uint32_t Size = 0u;

	bool IsValid() const { return Owner != nullptr; }
};
