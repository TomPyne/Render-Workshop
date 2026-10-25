#pragma once

#include <SurfMath.h>

#include <cstdint>

struct SpatialRenderingCollector_s;

// Must match DebugVertex_s in DebugDraw.hlsl
struct DebugVertex_s
{
	float3 Pos;
	uint32_t Color;
};
static_assert(sizeof(DebugVertex_s) == 16, "Must match DebugVertex_s in DebugDraw.hlsl");

enum class DebugPrimitive_e : uint32_t
{
	Lines = 0,
	Triangles,
	Count,
};

enum class DebugDepthMode_e : uint32_t
{
	DepthTested = 0,
	OnTop,
	Count,
};

uint32_t PackDebugColor(const float4& Color);

void DebugLine(SpatialRenderingCollector_s& Collector, const float3& A, const float3& B, const float4& Color, DebugDepthMode_e Mode = DebugDepthMode_e::DepthTested);
void DebugTriangle(SpatialRenderingCollector_s& Collector, const float3& A, const float3& B, const float3& C, const float4& Color, DebugDepthMode_e Mode = DebugDepthMode_e::DepthTested);
void DebugBox(SpatialRenderingCollector_s& Collector, const float3& Center, const float3& Extents, const quat& Rotation, const float4& Color, bool Solid = false, DebugDepthMode_e Mode = DebugDepthMode_e::DepthTested);
void DebugSphere(SpatialRenderingCollector_s& Collector, const float3& Center, float Radius, const float4& Color, bool Solid = false, DebugDepthMode_e Mode = DebugDepthMode_e::DepthTested, uint32_t Segments = 16u);
void DebugAxes(SpatialRenderingCollector_s& Collector, const matrix& Transform, float Length = 1.0f, DebugDepthMode_e Mode = DebugDepthMode_e::DepthTested);
