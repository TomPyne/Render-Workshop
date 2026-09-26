#pragma once

#include "FrameBufferAlloc.h"

#include <memory>
#include <new>
#include <Render/Buffers.h>
#include <thread>
#include <type_traits>
#include <vector>

// Records per frame constants into CPU pages, GPUContext_s::Execute uploads them to GPU memory before any recorded
// command runs and FrameBufferAlloc_s binds are resolved to their final GPU address at replay.
struct FrameBuffer_s
{
	static constexpr uint32_t PageSize = 256u * 1024u;
	static constexpr uint32_t Alignment = 256u;
	static constexpr uint32_t MaxAllocSize = 64u * 1024u;

	FrameBuffer_s() = default;
	FrameBuffer_s(const FrameBuffer_s&) = delete;
	FrameBuffer_s& operator=(const FrameBuffer_s&) = delete;

	FrameBufferAlloc_s Alloc(const void* Data, uint32_t Size);

	template<typename T>
	FrameBufferAlloc_s Alloc(const T& Data)
	{
		static_assert(std::is_trivially_copyable_v<T>, "FrameBuffer data must be trivially copyable");
		static_assert(sizeof(T) <= MaxAllocSize, "FrameBuffer allocs are limited to the 64KB CBV size");
		return Alloc(&Data, sizeof(T));
	}

	// The returned memory is only uploaded when the owning GPUContext_s executes, writes after that are lost
	void* AllocRaw(uint32_t Size, FrameBufferAlloc_s& OutHandle);

	template<typename T>
	T* Alloc(FrameBufferAlloc_s& OutHandle)
	{
		static_assert(std::is_trivially_copyable_v<T>, "FrameBuffer data must be trivially copyable");
		static_assert(sizeof(T) <= MaxAllocSize, "FrameBuffer allocs are limited to the 64KB CBV size");
		return new (AllocRaw(sizeof(T), OutHandle)) T();
	}

	uint32_t GetUsedSize() const;
	void GatherUploadSpans(size_t BaseOffset, std::vector<rl::ConstantUploadSpan_s>& OutSpans) const;
	void SetUploaded(rl::GPUAddress_t InGPUBase);

	rl::GPUAddress_t Resolve(const FrameBufferAlloc_s& Handle) const;

private:
	struct Page_s
	{
		std::unique_ptr<uint8_t[]> Memory;
		uint32_t Used = 0u;
	};

	std::vector<Page_s> Pages;
	rl::GPUAddress_t GPUBase = rl::GPUAddress_t::INVALID;
	bool Sealed = false;
	// Bound on first alloc so a FrameBuffer can be created on one thread and handed to a worker
	std::thread::id OwnerThread = {};
};
