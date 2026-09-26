#include "JobSystem.h"

#include "Logging/Logging.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

//#define JOBS_FORCE_SINGLE_THREADED

#define JOB_PROFILE_SCOPE(Name) (void)(Name)

namespace
{
	constexpr uint32_t PoolCapacity = 4096;
	constexpr uint64_t ClaimedBit = 1;

	enum class JobKind_e : uint8_t
	{
		Single,
		ParallelFor,
	};

	struct alignas(64) JobSlot_s
	{
		// Generation and Claimed share one word so a claim can't land on a recycled slot
		std::atomic<uint64_t> State = 0;	// high 32: Generation, bit 0: Claimed

		// Owner ref + queued tickets + helpers. The slot is freed when this reaches 0
		std::atomic<uint32_t> Refs = 0;

		// 64 bit so claims past the end can't wrap back onto chunk 0
		std::atomic<uint64_t> NextChunk = 0;
		std::atomic<uint32_t> ChunksRemaining = 0;
		uint32_t Count = 0;
		uint32_t ChunkSize = 0;
		uint32_t ChunkCount = 0;

		JobKind_e Kind = JobKind_e::Single;
		const char* DebugName = nullptr;

		std::move_only_function<void()> SingleFn;
		std::move_only_function<void(uint32_t, uint32_t) const> RangeFn;
	};

	struct Ticket_s
	{
		uint32_t SlotIndex = 0;
		uint32_t Generation = 0;
	};

	struct JobQueue_s
	{
		void Push(Ticket_s Ticket)
		{
			{
				std::lock_guard Lock(Mutex);
				Tickets.push_back(Ticket);
			}

			WakeCondition.notify_one();
		}

		void PushN(Ticket_s Ticket, uint32_t Num)
		{
			{
				std::lock_guard Lock(Mutex);
				Tickets.insert(Tickets.end(), Num, Ticket);
			}

			WakeCondition.notify_all();
		}

		// Returns false once stopped, even if tickets remain
		bool PopBlocking(Ticket_s& OutTicket)
		{
			std::unique_lock Lock(Mutex);
			WakeCondition.wait(Lock, [this] { return Stopping || !Tickets.empty(); });

			if (Stopping)
			{
				return false;
			}

			OutTicket = Tickets.front();
			Tickets.pop_front();
			return true;
		}

		void Stop()
		{
			{
				std::lock_guard Lock(Mutex);
				Stopping = true;
			}

			WakeCondition.notify_all();
		}

		void DiscardAll()
		{
			std::lock_guard Lock(Mutex);
			Tickets.clear();
			Stopping = false;
		}

	private:
		std::mutex Mutex;
		std::condition_variable WakeCondition;
		std::deque<Ticket_s> Tickets;
		bool Stopping = false;
	};

	bool Initialised = false;
	bool SingleThreaded = false;
	uint32_t WorkerCount = 0;

	std::unique_ptr<JobSlot_s[]> Pool;
	std::vector<uint32_t> FreeSlots;
	std::mutex FreeSlotsMutex;

	JobQueue_s Queue;
	std::vector<std::thread> Workers;

	uint32_t ComputeWorkerCount(const JobSystemDesc_s& Desc)
	{
		if (Desc.WorkerCountOverride > 0)
		{
			return Desc.WorkerCountOverride;
		}

		// hardware_concurrency can return 0 when it can't be determined
		const uint32_t HardwareThreads = std::thread::hardware_concurrency();
		return HardwareThreads > Desc.ReservedThreads ? HardwareThreads - Desc.ReservedThreads : 1;
	}

	uint32_t ResolveChunkSize(uint32_t Count, uint32_t ChunkSize)
	{
		if (ChunkSize > 0)
		{
			return ChunkSize;
		}

		// +1 for the thread that waits and helps
		const uint32_t ChunkCount = std::min(Count, (WorkerCount + 1) * 4);
		return (Count + ChunkCount - 1) / ChunkCount;
	}

	// noexcept so a throwing job terminates at the throw site instead of unwinding through the pool
	void RunSingle(std::move_only_function<void()>& Fn, const char* DebugName) noexcept
	{
		JOB_PROFILE_SCOPE(DebugName);
		Fn();
	}

	void RunRange(const std::move_only_function<void(uint32_t, uint32_t) const>& Fn, uint32_t Begin, uint32_t End, const char* DebugName) noexcept
	{
		JOB_PROFILE_SCOPE(DebugName);
		Fn(Begin, End);
	}

	uint64_t PackState(uint32_t Generation, bool Claimed)
	{
		return (static_cast<uint64_t>(Generation) << 32) | (Claimed ? ClaimedBit : 0);
	}

	uint32_t GenerationOf(uint64_t State)
	{
		return static_cast<uint32_t>(State >> 32);
	}

	// Generation 0 is reserved for default handles, which are always complete
	uint32_t NextGeneration(uint32_t Generation)
	{
		return Generation == UINT32_MAX ? 1 : Generation + 1;
	}

	void CreatePool()
	{
		Pool = std::make_unique<JobSlot_s[]>(PoolCapacity);

		FreeSlots.clear();
		FreeSlots.reserve(PoolCapacity);

		// Reversed so slots are handed out from index 0
		for (uint32_t SlotIndex = PoolCapacity; SlotIndex > 0; --SlotIndex)
		{
			Pool[SlotIndex - 1].State.store(PackState(1, false), std::memory_order_relaxed);
			FreeSlots.push_back(SlotIndex - 1);
		}
	}

	void DestroyPool()
	{
		Pool.reset();
		FreeSlots.clear();
	}

	// The caller must set Refs and fill in the job before publishing the slot
	uint32_t AllocateSlot()
	{
		std::lock_guard Lock(FreeSlotsMutex);

		ASSERTMSG(!FreeSlots.empty(), "Job pool exhausted (%u slots)", PoolCapacity);

		const uint32_t SlotIndex = FreeSlots.back();
		FreeSlots.pop_back();
		return SlotIndex;
	}

	void ReleaseRef(uint32_t SlotIndex)
	{
		if (Pool[SlotIndex].Refs.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			std::lock_guard Lock(FreeSlotsMutex);
			FreeSlots.push_back(SlotIndex);
		}
	}

	// Increment only if nonzero. A plain fetch_add could revive a slot already on the free list,
	// and the matching release would then free it twice
	bool TryAcquireRef(uint32_t SlotIndex)
	{
		std::atomic<uint32_t>& Refs = Pool[SlotIndex].Refs;

		uint32_t Current = Refs.load(std::memory_order_relaxed);
		while (Current != 0)
		{
			if (Refs.compare_exchange_weak(Current, Current + 1, std::memory_order_acquire, std::memory_order_relaxed))
			{
				return true;
			}
		}

		return false;
	}

	// Callables are destroyed before completion is published, so by-value captures are released
	// by the time a waiter sees the job as complete
	void CompleteJob(uint32_t SlotIndex)
	{
		JobSlot_s& Slot = Pool[SlotIndex];

		Slot.SingleFn = nullptr;
		Slot.RangeFn = nullptr;

		const uint32_t Generation = GenerationOf(Slot.State.load(std::memory_order_relaxed));
		Slot.State.store(PackState(NextGeneration(Generation), false), std::memory_order_release);
		Slot.State.notify_all();

		ReleaseRef(SlotIndex);
	}

	// Called while holding a ref on the slot
	void TryRunSingle(uint32_t SlotIndex, uint32_t Generation)
	{
		JobSlot_s& Slot = Pool[SlotIndex];

		uint64_t Expected = PackState(Generation, false);
		if (Slot.State.compare_exchange_strong(Expected, PackState(Generation, true), std::memory_order_acquire, std::memory_order_relaxed))
		{
			RunSingle(Slot.SingleFn, Slot.DebugName);
			CompleteJob(SlotIndex);
		}
	}

	// Called while holding a ref on the slot
	void RunChunks(uint32_t SlotIndex)
	{
		JobSlot_s& Slot = Pool[SlotIndex];

		for (;;)
		{
			const uint64_t Chunk = Slot.NextChunk.fetch_add(1, std::memory_order_relaxed);
			if (Chunk >= Slot.ChunkCount)
			{
				return;
			}

			const uint32_t Begin = static_cast<uint32_t>(Chunk * Slot.ChunkSize);
			const uint32_t End = Begin + std::min(Slot.ChunkSize, Slot.Count - Begin);
			RunRange(Slot.RangeFn, Begin, End, Slot.DebugName);

			if (Slot.ChunksRemaining.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				CompleteJob(SlotIndex);
				return;
			}
		}
	}

	void WorkerMain(uint32_t WorkerIndex)
	{
		const std::wstring ThreadName = L"Job Worker " + std::to_wstring(WorkerIndex);
		SetThreadDescription(GetCurrentThread(), ThreadName.c_str());

		Ticket_s Ticket;
		while (Queue.PopBlocking(Ticket))
		{
			// The ticket's ref keeps the slot from being recycled while it's read
			if (Pool[Ticket.SlotIndex].Kind == JobKind_e::Single)
			{
				TryRunSingle(Ticket.SlotIndex, Ticket.Generation);
			}
			else
			{
				RunChunks(Ticket.SlotIndex);
			}

			ReleaseRef(Ticket.SlotIndex);
		}
	}

	// Release so a thread that claims through a handle rather than the queue sees the filled slot
	uint32_t PublishSlot(JobSlot_s& Slot)
	{
		const uint32_t Generation = GenerationOf(Slot.State.load(std::memory_order_relaxed));
		Slot.State.store(PackState(Generation, false), std::memory_order_release);
		return Generation;
	}
}

void JobSystemInit(const JobSystemDesc_s& Desc)
{
	ASSERTMSG(!Initialised, "Job system is already initialised");

#ifdef JOBS_FORCE_SINGLE_THREADED
	SingleThreaded = true;
#else
	SingleThreaded = Desc.SingleThreaded;
#endif

	WorkerCount = SingleThreaded ? 0 : ComputeWorkerCount(Desc);

	if (!SingleThreaded)
	{
		CreatePool();

		Workers.reserve(WorkerCount);
		for (uint32_t WorkerIndex = 0; WorkerIndex < WorkerCount; ++WorkerIndex)
		{
			Workers.emplace_back(WorkerMain, WorkerIndex);
		}
	}

	Initialised = true;
}

void JobSystemShutdown()
{
	ASSERTMSG(Initialised, "Job system is not initialised");

	if (!SingleThreaded)
	{
		// Running jobs finish first. Anything they wait on that is still queued gets claimed inline
		Queue.Stop();

		for (std::thread& Worker : Workers)
		{
			Worker.join();
		}

		Workers.clear();
		Queue.DiscardAll();
	}

	DestroyPool();

	WorkerCount = 0;
	Initialised = false;
}

uint32_t JobSystemGetWorkerCount()
{
	return WorkerCount;
}

JobHandle_s JobSubmit(std::move_only_function<void()> Fn, const char* DebugName)
{
	ASSERTMSG(Initialised, "Job system is not initialised");
	ASSERTMSG(static_cast<bool>(Fn), "Submitted an empty job");

	if (SingleThreaded)
	{
		RunSingle(Fn, DebugName);
		return {};
	}

	const uint32_t SlotIndex = AllocateSlot();
	JobSlot_s& Slot = Pool[SlotIndex];

	Slot.Kind = JobKind_e::Single;
	Slot.DebugName = DebugName;
	Slot.SingleFn = std::move(Fn);

	// Owner + one ticket
	Slot.Refs.store(2, std::memory_order_relaxed);

	const uint32_t Generation = PublishSlot(Slot);
	Queue.Push({ SlotIndex, Generation });

	return { SlotIndex, Generation };
}

void JobSubmitDetached(std::move_only_function<void()> Fn, const char* DebugName)
{
	(void)JobSubmit(std::move(Fn), DebugName);
}

JobHandle_s JobParallelFor(uint32_t Count, uint32_t ChunkSize, std::move_only_function<void(uint32_t Begin, uint32_t End) const> Fn, const char* DebugName)
{
	ASSERTMSG(Initialised, "Job system is not initialised");
	ASSERTMSG(static_cast<bool>(Fn), "Submitted an empty parallel for");

	if (Count == 0)
	{
		return {};
	}

	const uint32_t ResolvedChunkSize = ResolveChunkSize(Count, ChunkSize);

	if (SingleThreaded)
	{
		for (uint32_t Begin = 0; Begin < Count;)
		{
			// Written to avoid overflow when Count is near UINT32_MAX
			const uint32_t End = Begin + std::min(ResolvedChunkSize, Count - Begin);
			RunRange(Fn, Begin, End, DebugName);
			Begin = End;
		}

		return {};
	}

	const uint32_t ChunkCount = (Count - 1) / ResolvedChunkSize + 1;
	const uint32_t TicketCount = std::min(ChunkCount, WorkerCount);

	const uint32_t SlotIndex = AllocateSlot();
	JobSlot_s& Slot = Pool[SlotIndex];

	Slot.Kind = JobKind_e::ParallelFor;
	Slot.DebugName = DebugName;
	Slot.RangeFn = std::move(Fn);
	Slot.Count = Count;
	Slot.ChunkSize = ResolvedChunkSize;
	Slot.ChunkCount = ChunkCount;
	Slot.NextChunk.store(0, std::memory_order_relaxed);
	Slot.ChunksRemaining.store(ChunkCount, std::memory_order_relaxed);

	// Owner + one per ticket
	Slot.Refs.store(1 + TicketCount, std::memory_order_relaxed);

	const uint32_t Generation = PublishSlot(Slot);
	Queue.PushN({ SlotIndex, Generation }, TicketCount);

	return { SlotIndex, Generation };
}

void JobWaitOrHelp(JobHandle_s Handle)
{
	ASSERTMSG(Initialised, "Job system is not initialised");

	if (Handle.Generation == 0 || SingleThreaded)
	{
		return;
	}

	ASSERTMSG(Handle.Index < PoolCapacity, "Invalid job handle");

	JobSlot_s& Slot = Pool[Handle.Index];

	// The ref stops the slot being recycled while Kind is read and the job is helped with
	if (!TryAcquireRef(Handle.Index))
	{
		return;
	}

	if (GenerationOf(Slot.State.load(std::memory_order_acquire)) == Handle.Generation)
	{
		if (Slot.Kind == JobKind_e::Single)
		{
			TryRunSingle(Handle.Index, Handle.Generation);
		}
		else
		{
			RunChunks(Handle.Index);
		}
	}

	ReleaseRef(Handle.Index);

	// Whatever is left is running on other threads
	for (;;)
	{
		const uint64_t State = Slot.State.load(std::memory_order_acquire);
		if (GenerationOf(State) != Handle.Generation)
		{
			return;
		}

		Slot.State.wait(State, std::memory_order_acquire);
	}
}

bool JobIsComplete(JobHandle_s Handle)
{
	if (Handle.Generation == 0 || !Pool)
	{
		return true;
	}

	ASSERTMSG(Handle.Index < PoolCapacity, "Invalid job handle");

	// Acquire pairs with the release in CompleteJob, so the job's writes are visible
	const uint64_t State = Pool[Handle.Index].State.load(std::memory_order_acquire);
	return GenerationOf(State) != Handle.Generation;
}
