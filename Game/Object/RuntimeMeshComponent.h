#pragma once

#include "SpatialObjectComponent.h"

#include <Render/RenderTypes.h>

#include <SurfMath.h>

#include <vector>

struct RuntimeMeshDesc_s
{
	std::vector<float3>* Positions;
	std::vector<uint32_t>* Indices;
};

class RuntimeMeshComponent_c : public SpatialObjectComponent_c
{
public:
	virtual ~RuntimeMeshComponent_c() = default;

	void UpdateMesh(const RuntimeMeshDesc_s& Desc);

	rl::VertexBufferPtr PositionBuffer = {};
	rl::IndexBufferPtr IndexBuffer = {};
};