#pragma once

#include <Render/Render.h>

struct ShapeMeshPos
{
	tpr::VertexBuffer_t BindBuffers[1] = {};
	uint32_t BindStrides[1] = {};
	uint32_t BindOffsets[1] = {};

	tpr::VertexBufferPtr PositionBuffer = {};
	tpr::IndexBufferPtr IndexBuffer = {};
	uint32_t IndexCount = 0;
	tpr::RenderFormat IndexFormat = tpr::RenderFormat::UNKNOWN;

	std::vector<tpr::InputElementDesc> InputDesc;
};

struct ShapeMeshPosNormal
{
	tpr::VertexBuffer_t BindBuffers[2] = {};
	uint32_t BindStrides[2] = {};
	uint32_t BindOffsets[2] = {};

	tpr::VertexBufferPtr PositionBuffer = {};
	tpr::VertexBufferPtr NormalBuffer = {};
	tpr::IndexBufferPtr IndexBuffer = {};
	uint32_t IndexCount = 0;
	tpr::RenderFormat IndexFormat = tpr::RenderFormat::UNKNOWN;

	std::vector<tpr::InputElementDesc> InputDesc;
};

ShapeMeshPos CreateCubeMeshPos(float Scale);
ShapeMeshPosNormal CreateCubeMeshPosNormal(float Scale);

ShapeMeshPos CreateTileMeshPos(float Scale, uint32_t Dimension);

ShapeMeshPos CreateSphereMeshPos(float Scale, uint32_t Slices, uint32_t Stacks);
ShapeMeshPosNormal CreateSphereMeshPosNormal(float Scale, uint32_t Slices, uint32_t Stacks);