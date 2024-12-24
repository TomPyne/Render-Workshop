#pragma once

#include <Render/Render.h>

#include <vector>

struct ModelVertexBuffer_s
{
	tpr::VertexBufferPtr Buffer = tpr::VertexBuffer_t::INVALID;
	uint32_t Stride = 0u;
	uint32_t Offset = 0u;
};

struct ModelIndexBuffer_t
{
	tpr::IndexBufferPtr Buffer = tpr::IndexBuffer_t::INVALID;
	tpr::RenderFormat Format = tpr::RenderFormat::UNKNOWN;
	uint32_t offset = 0;
	uint32_t count = 0;
};

struct Mesh_s
{
	ModelVertexBuffer_s Position;
	ModelVertexBuffer_s Normal;
	ModelVertexBuffer_s Texcoord;
	ModelVertexBuffer_s Color;
	ModelIndexBuffer_t Index;

	std::vector<tpr::InputElementDesc> InputDesc;
};

struct Model_s
{
	std::vector<Mesh_s> Meshes;
};

bool LoadModelFromOBJ(const char* OBJPath, Model_s& OutModel);