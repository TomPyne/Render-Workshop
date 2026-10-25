#include "FrameBuffer.h"

#include <cstring>
#include <Logging/Logging.h>

FrameBufferAlloc_s FrameBuffer_s::Alloc(const void* Data, uint32_t Size)
{
	FrameBufferAlloc_s Handle;
	memcpy(AllocRaw(Size, Handle), Data, Size);

	return Handle;
}

void* FrameBuffer_s::AllocRaw(uint32_t Size, FrameBufferAlloc_s& OutHandle)
{
	ASSERTMSG(Size > 0u && Size <= MaxAllocSize, "FrameBuffer alloc size must be in (0, 64KB]");

	return AllocInternal(Size, Alignment, OutHandle);
}

FrameBufferAlloc_s FrameBuffer_s::AllocData(const void* Data, uint32_t Size)
{
	FrameBufferAlloc_s Handle;
	memcpy(AllocDataRaw(Size, Handle), Data, Size);

	return Handle;
}

void* FrameBuffer_s::AllocDataRaw(uint32_t Size, FrameBufferAlloc_s& OutHandle)
{
	ASSERTMSG(Size > 0u, "FrameBuffer data alloc size must be non zero");

	return AllocInternal(Size, DataAlignment, OutHandle);
}

void* FrameBuffer_s::AllocInternal(uint32_t Size, uint32_t AllocAlignment, FrameBufferAlloc_s& OutHandle)
{
	ASSERTMSG(!Sealed, "FrameBuffer has already been uploaded");

	if (OwnerThread == std::thread::id())
	{
		OwnerThread = std::this_thread::get_id();
	}
	ASSERTMSG(std::this_thread::get_id() == OwnerThread, "FrameBuffer allocated from a thread that doesn't own it");

	uint32_t Offset = 0u;
	bool NeedsPage = Pages.empty();
	if (!NeedsPage)
	{
		Offset = (Pages.back().Used + AllocAlignment - 1u) & ~(AllocAlignment - 1u);
		NeedsPage = Offset + Size > Pages.back().Size;
	}

	// Allocations never straddle pages so each page uploads as a single span. Page sizes are multiples of PageSize,
	// which keeps every page base 256 byte aligned for CBV binds.
	if (NeedsPage)
	{
		const uint32_t BaseOffset = Pages.empty() ? 0u : Pages.back().BaseOffset + Pages.back().Size;
		const uint32_t NewPageSize = Size <= PageSize ? PageSize : (Size + PageSize - 1u) & ~(PageSize - 1u);

		Page_s& NewPage = Pages.emplace_back();
		NewPage.Memory = std::make_unique_for_overwrite<uint8_t[]>(NewPageSize);
		NewPage.BaseOffset = BaseOffset;
		NewPage.Size = NewPageSize;
		Offset = 0u;
	}

	Page_s& Page = Pages.back();
	Page.Used = Offset + Size;

	OutHandle.Owner = this;
	OutHandle.Offset = Page.BaseOffset + Offset;
	OutHandle.Size = Size;

	return Page.Memory.get() + Offset;
}

uint32_t FrameBuffer_s::GetUsedSize() const
{
	return Pages.empty() ? 0u : Pages.back().BaseOffset + Pages.back().Used;
}

void FrameBuffer_s::GatherUploadSpans(size_t BaseOffset, std::vector<rl::ConstantUploadSpan_s>& OutSpans) const
{
	ASSERTMSG(!Sealed, "FrameBuffer has already been uploaded");
	ASSERTMSG((BaseOffset % Alignment) == 0u, "FrameBuffer base offset must be 256 byte aligned");

	OutSpans.reserve(OutSpans.size() + Pages.size());

	for (const Page_s& Page : Pages)
	{
		rl::ConstantUploadSpan_s& Span = OutSpans.emplace_back();
		Span.Data = Page.Memory.get();
		Span.Size = Page.Used;
		Span.DstOffset = BaseOffset + Page.BaseOffset;
	}
}

void FrameBuffer_s::SetUploaded(rl::GPUAddress_t InGPUBase)
{
	ASSERTMSG(!Sealed, "FrameBuffer has already been uploaded");
	ASSERTMSG(rl::IsValid(InGPUBase), "FrameBuffer uploaded to an invalid address");

	GPUBase = InGPUBase;
	Sealed = true;
}

rl::GPUAddress_t FrameBuffer_s::Resolve(const FrameBufferAlloc_s& Handle) const
{
	ASSERTMSG(Handle.Owner == this, "FrameBufferAlloc_s resolved against a FrameBuffer that didn't allocate it");
	ASSERTMSG(Sealed, "FrameBufferAlloc_s bound but its FrameBuffer was never uploaded by the executing GPUContext");
	ASSERTMSG(Handle.Offset + Handle.Size <= GetUsedSize(), "FrameBufferAlloc_s is out of range");

	return static_cast<rl::GPUAddress_t>(static_cast<uint64_t>(GPUBase) + Handle.Offset);
}
