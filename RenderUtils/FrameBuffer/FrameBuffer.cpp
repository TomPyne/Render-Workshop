#include "FrameBuffer.h"

#include <cstring>
#include <Logging/Logging.h>

FrameBufferAlloc_t FrameBuffer_s::Alloc(const void* Data, uint32_t Size)
{
	FrameBufferAlloc_t Handle;
	memcpy(AllocRaw(Size, Handle), Data, Size);

	return Handle;
}

void* FrameBuffer_s::AllocRaw(uint32_t Size, FrameBufferAlloc_t& OutHandle)
{
	ASSERTMSG(!Sealed, "FrameBuffer has already been uploaded");
	ASSERTMSG(Size > 0u && Size <= MaxAllocSize, "FrameBuffer alloc size must be in (0, 64KB]");

	if (OwnerThread == std::thread::id())
	{
		OwnerThread = std::this_thread::get_id();
	}
	ASSERTMSG(std::this_thread::get_id() == OwnerThread, "FrameBuffer allocated from a thread that doesn't own it");

	uint32_t Offset = Pages.empty() ? PageSize : (Pages.back().Used + Alignment - 1u) & ~(Alignment - 1u);

	// Allocations never straddle pages so each page uploads as a single span
	if (Offset + Size > PageSize)
	{
		Page_s& NewPage = Pages.emplace_back();
		NewPage.Memory = std::make_unique_for_overwrite<uint8_t[]>(PageSize);
		Offset = 0u;
	}

	Page_s& Page = Pages.back();
	Page.Used = Offset + Size;

	OutHandle.Owner = this;
	OutHandle.Offset = static_cast<uint32_t>((Pages.size() - 1u) * PageSize) + Offset;
	OutHandle.Size = Size;

	return Page.Memory.get() + Offset;
}

uint32_t FrameBuffer_s::GetUsedSize() const
{
	return Pages.empty() ? 0u : static_cast<uint32_t>((Pages.size() - 1u) * PageSize) + Pages.back().Used;
}

void FrameBuffer_s::GatherUploadSpans(size_t BaseOffset, std::vector<rl::ConstantUploadSpan_s>& OutSpans) const
{
	ASSERTMSG(!Sealed, "FrameBuffer has already been uploaded");
	ASSERTMSG((BaseOffset % Alignment) == 0u, "FrameBuffer base offset must be 256 byte aligned");

	OutSpans.reserve(OutSpans.size() + Pages.size());

	for (size_t PageIndex = 0; PageIndex < Pages.size(); ++PageIndex)
	{
		const Page_s& Page = Pages[PageIndex];

		rl::ConstantUploadSpan_s& Span = OutSpans.emplace_back();
		Span.Data = Page.Memory.get();
		Span.Size = Page.Used;
		Span.DstOffset = BaseOffset + PageIndex * PageSize;
	}
}

void FrameBuffer_s::SetUploaded(rl::GPUAddress_t InGPUBase)
{
	ASSERTMSG(!Sealed, "FrameBuffer has already been uploaded");
	ASSERTMSG(rl::IsValid(InGPUBase), "FrameBuffer uploaded to an invalid address");

	GPUBase = InGPUBase;
	Sealed = true;
}

rl::GPUAddress_t FrameBuffer_s::Resolve(const FrameBufferAlloc_t& Handle) const
{
	ASSERTMSG(Handle.Owner == this, "FrameBufferAlloc_t resolved against a FrameBuffer that didn't allocate it");
	ASSERTMSG(Sealed, "FrameBufferAlloc_t bound but its FrameBuffer was never uploaded by the executing GPUContext");
	ASSERTMSG(Handle.Offset + Handle.Size <= GetUsedSize(), "FrameBufferAlloc_t is out of range");

	return static_cast<rl::GPUAddress_t>(static_cast<uint64_t>(GPUBase) + Handle.Offset);
}
