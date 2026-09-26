#pragma once

#include <cstdint>
#include <functional>

struct JobSystemDesc_s
{
	uint32_t ReservedThreads = 0;
	uint32_t WorkerCountOverride = 0;	// 0 = hardware threads minus ReservedThreads
	bool SingleThreaded = false;
};

struct JobHandle_s
{
	uint32_t Index = 0;
	uint32_t Generation = 0;
};

void JobSystemInit(const JobSystemDesc_s& Desc);
void JobSystemShutdown();

uint32_t JobSystemGetWorkerCount();

[[nodiscard]] JobHandle_s JobSubmit(std::move_only_function<void()> Fn, const char* DebugName = nullptr);

void JobSubmitDetached(std::move_only_function<void()> Fn, const char* DebugName = nullptr);

[[nodiscard]] JobHandle_s JobParallelFor(uint32_t Count, uint32_t ChunkSize, std::move_only_function<void(uint32_t Begin, uint32_t End) const> Fn, const char* DebugName = nullptr);

void JobWaitOrHelp(JobHandle_s Handle);
bool JobIsComplete(JobHandle_s Handle);
