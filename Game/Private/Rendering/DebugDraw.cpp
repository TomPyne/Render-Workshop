#include "Rendering/DebugDraw.h"

#include "Rendering/SpatialRenderingCollector.h"

#include <cmath>

static uint32_t PackUnorm8(float Value)
{
	return static_cast<uint32_t>(Clamp(Value, 0.0f, 1.0f) * 255.0f + 0.5f);
}

uint32_t PackDebugColor(const float4& Color)
{
	return PackUnorm8(Color.x) | (PackUnorm8(Color.y) << 8) | (PackUnorm8(Color.z) << 16) | (PackUnorm8(Color.w) << 24);
}

void DebugLine(SpatialRenderingCollector_s& Collector, const float3& A, const float3& B, const float4& Color, DebugDepthMode_e Mode)
{
	Collector.AddDebugLine(A, B, PackDebugColor(Color), Mode);
}

void DebugTriangle(SpatialRenderingCollector_s& Collector, const float3& A, const float3& B, const float3& C, const float4& Color, DebugDepthMode_e Mode)
{
	Collector.AddDebugTriangle(A, B, C, PackDebugColor(Color), Mode);
}

void DebugBox(SpatialRenderingCollector_s& Collector, const float3& Center, const float3& Extents, const quat& Rotation, const float4& Color, bool Solid, DebugDepthMode_e Mode)
{
	if (!Collector.IsDebugDrawEnabled())
		return;

	// Corner index bits select the sign per axis: bit 0 x, bit 1 y, bit 2 z
	float3 Corners[8];
	for (uint32_t i = 0; i < 8; ++i)
	{
		const float3 Local = float3((i & 1) ? Extents.x : -Extents.x, (i & 2) ? Extents.y : -Extents.y, (i & 4) ? Extents.z : -Extents.z);
		Corners[i] = Center + ::Rotate(Rotation, Local);
	}

	const uint32_t Packed = PackDebugColor(Color);

	if (Solid)
	{
		// Each face as a quad of corner indices, culling is off so winding doesn't matter
		static constexpr uint32_t Faces[6][4] =
		{
			{ 0, 1, 3, 2 }, { 4, 5, 7, 6 },
			{ 0, 1, 5, 4 }, { 2, 3, 7, 6 },
			{ 0, 2, 6, 4 }, { 1, 3, 7, 5 },
		};

		for (const auto& Face : Faces)
		{
			Collector.AddDebugTriangle(Corners[Face[0]], Corners[Face[1]], Corners[Face[2]], Packed, Mode);
			Collector.AddDebugTriangle(Corners[Face[0]], Corners[Face[2]], Corners[Face[3]], Packed, Mode);
		}
	}
	else
	{
		static constexpr uint32_t Edges[12][2] =
		{
			{ 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 },
			{ 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 },
			{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
		};

		for (const auto& Edge : Edges)
		{
			Collector.AddDebugLine(Corners[Edge[0]], Corners[Edge[1]], Packed, Mode);
		}
	}
}

void DebugSphere(SpatialRenderingCollector_s& Collector, const float3& Center, float Radius, const float4& Color, bool Solid, DebugDepthMode_e Mode, uint32_t Segments)
{
	if (!Collector.IsDebugDrawEnabled())
		return;

	Segments = Max(Segments, 4u);
	const uint32_t Packed = PackDebugColor(Color);

	if (Solid)
	{
		const uint32_t Rings = Max(Segments / 2u, 2u);

		auto Point = [&](uint32_t Ring, uint32_t Segment)
		{
			const float Theta = K_PI * static_cast<float>(Ring) / static_cast<float>(Rings);
			const float Phi = 2.0f * K_PI * static_cast<float>(Segment) / static_cast<float>(Segments);
			const float SinTheta = sinf(Theta);
			return Center + float3(SinTheta * cosf(Phi), cosf(Theta), SinTheta * sinf(Phi)) * Radius;
		};

		for (uint32_t Ring = 0; Ring < Rings; ++Ring)
		{
			for (uint32_t Segment = 0; Segment < Segments; ++Segment)
			{
				const float3 P00 = Point(Ring, Segment);
				const float3 P01 = Point(Ring, Segment + 1);
				const float3 P10 = Point(Ring + 1, Segment);
				const float3 P11 = Point(Ring + 1, Segment + 1);

				// The pole rows would otherwise emit a degenerate triangle per segment
				if (Ring > 0)
				{
					Collector.AddDebugTriangle(P00, P01, P11, Packed, Mode);
				}
				if (Ring + 1 < Rings)
				{
					Collector.AddDebugTriangle(P00, P11, P10, Packed, Mode);
				}
			}
		}
	}
	else
	{
		// Three great circles, one per axis plane
		for (uint32_t Segment = 0; Segment < Segments; ++Segment)
		{
			const float A0 = 2.0f * K_PI * static_cast<float>(Segment) / static_cast<float>(Segments);
			const float A1 = 2.0f * K_PI * static_cast<float>(Segment + 1) / static_cast<float>(Segments);
			const float C0 = cosf(A0) * Radius, S0 = sinf(A0) * Radius;
			const float C1 = cosf(A1) * Radius, S1 = sinf(A1) * Radius;

			Collector.AddDebugLine(Center + float3(C0, S0, 0.0f), Center + float3(C1, S1, 0.0f), Packed, Mode);
			Collector.AddDebugLine(Center + float3(C0, 0.0f, S0), Center + float3(C1, 0.0f, S1), Packed, Mode);
			Collector.AddDebugLine(Center + float3(0.0f, C0, S0), Center + float3(0.0f, C1, S1), Packed, Mode);
		}
	}
}

void DebugAxes(SpatialRenderingCollector_s& Collector, const matrix& Transform, float Length, DebugDepthMode_e Mode)
{
	if (!Collector.IsDebugDrawEnabled())
		return;

	const float3 Origin = Transform.r[3].xyz;

	Collector.AddDebugLine(Origin, Origin + Transform.r[0].xyz * Length, PackDebugColor(float4(1.0f, 0.0f, 0.0f, 1.0f)), Mode);
	Collector.AddDebugLine(Origin, Origin + Transform.r[1].xyz * Length, PackDebugColor(float4(0.0f, 1.0f, 0.0f, 1.0f)), Mode);
	Collector.AddDebugLine(Origin, Origin + Transform.r[2].xyz * Length, PackDebugColor(float4(0.0f, 0.0f, 1.0f, 1.0f)), Mode);
}
