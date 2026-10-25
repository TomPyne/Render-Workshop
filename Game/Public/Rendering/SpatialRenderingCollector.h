#pragma once

#include "Rendering/DebugDraw.h"
#include "Rendering/DistanceFieldScene.h"

#include <Render/RenderTypes.h>
#include <Render/Raytracing.h>
#include <RenderUtils/FrameBuffer/FrameBuffer.h>
#include <Shared/Types/Enum.h>

#include <type_traits>
#include <vector>

struct SpatialRenderingBatch_s
{
	FrameBufferAlloc_s DynamicUniforms;
	rl::ConstantBuffer_t MeshUniforms;
	rl::ConstantBuffer_t MaterialUniforms;
	rl::IndexBuffer_t IndexBuffer;
	rl::RenderFormat IndexBufferFormat;
	uint32_t IndexOffset;
	uint32_t IndexCount;
	rl::GraphicsPipelineState_t PSO;
};

struct SpatialRenderingMeshPass_s
{
	std::vector<SpatialRenderingBatch_s> Batches;

	SpatialRenderingBatch_s& AddBatch()
	{
		return Batches.emplace_back();
	}
};

enum class SpatialShader_t : uint32_t
{
	INVALID = 0,
};

enum class SpatialShaderPass_e : uint32_t
{
	NONE = 0,
	MAIN,
	TRANSLUCENT,
	COUNT,
};

enum class CollectorFlags_e : uint32_t
{
	NONE = 0,
	RAYTRACING_INSTANCES = 1 << 0,
	DISTANCE_FIELD_INSTANCES = 1 << 1,
	DEBUG_DRAW = 1 << 2,
};
ENUM_FLAGS(CollectorFlags_e);

struct SpatialRenderingCollector_s
{
	SpatialRenderingCollector_s(FrameBuffer_s& InFrameBuffer, uint64_t InFrameIndex, CollectorFlags_e InFlags = CollectorFlags_e::NONE)
		: FrameIndex(InFrameIndex)
		, FrameBuffer(InFrameBuffer)
		, Flags(InFlags)
	{}

	template<typename T>
	FrameBufferAlloc_s Alloc(const T& Data)
	{
		static_assert(std::is_trivially_copyable_v<T>, "FrameBuffer data must be trivially copyable");
		return Alloc(&Data, sizeof(T));
	}

	FrameBufferAlloc_s Alloc(const void* Data, uint32_t Size);

	// Read in shaders as a ByteAddressBuffer, not limited to the CBV size
	FrameBufferAlloc_s AllocData(const void* Data, uint32_t Size);

	SpatialRenderingBatch_s& AddBatch(SpatialShaderPass_e Pass)
	{
		return Passes[static_cast<size_t>(Pass)].AddBatch();
	}

	rl::RaytracingInstance& AddRaytracingInstance()
	{
		return RaytracingInstances.emplace_back();
	}

	DistanceFieldInstance_s& AddDistanceFieldInstance()
	{
		return DistanceFieldInstances.emplace_back();
	}

	bool IsDebugDrawEnabled() const { return HasEnumFlags(Flags, CollectorFlags_e::DEBUG_DRAW); }

	// Color is packed RGBA8, see PackDebugColor
	void AddDebugLine(const float3& A, const float3& B, uint32_t Color, DebugDepthMode_e Mode = DebugDepthMode_e::DepthTested);
	void AddDebugTriangle(const float3& A, const float3& B, const float3& C, uint32_t Color, DebugDepthMode_e Mode = DebugDepthMode_e::DepthTested);

	const std::vector<DebugVertex_s>& GetDebugVertices(DebugPrimitive_e Primitive, DebugDepthMode_e Mode) const
	{
		return DebugVertices[static_cast<size_t>(Primitive)][static_cast<size_t>(Mode)];
	}

	const std::vector<rl::RaytracingInstance>& GetRaytracingInstances() const { return RaytracingInstances; }
	const std::vector<DistanceFieldInstance_s>& GetDistanceFieldInstances() const { return DistanceFieldInstances; }

	const std::vector<SpatialRenderingBatch_s>& GetBatches(SpatialShaderPass_e Pass) const
	{
		return Passes[static_cast<size_t>(Pass)].Batches;
	}

	uint32_t GetNumBatches() const;

	const uint64_t FrameIndex;
	const CollectorFlags_e Flags;

private:
	FrameBuffer_s& FrameBuffer;

	SpatialRenderingMeshPass_s Passes[static_cast<size_t>(SpatialShaderPass_e::COUNT)];
	std::vector<rl::RaytracingInstance> RaytracingInstances;
	std::vector<DistanceFieldInstance_s> DistanceFieldInstances;
	std::vector<DebugVertex_s> DebugVertices[static_cast<size_t>(DebugPrimitive_e::Count)][static_cast<size_t>(DebugDepthMode_e::Count)];
};
