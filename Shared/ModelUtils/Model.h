#pragma once

#include "FileUtils/PathUtils.h"
#include "TextureUtils/TextureManager.h"
#include <Render/Render.h>
#include <SurfMath.h>
#include <vector>

struct MeshAsset_s
{
	wchar_t MaterialPath[PathUtils::MaxPath];

	uint32_t IndexOffset;
	uint32_t IndexCount;

	uint32_t MeshletCount;	
	struct Meshlet_s
	{
		uint32_t VertCount;
		uint32_t VertOffset;
		uint32_t PrimCount;
		uint32_t PrimOffset;
	};
	std::vector<Meshlet_s> Meshlets;
};

struct ModelAsset_s
{
	uint32_t VertexCount = 0;
	uint32_t IndexCount = 0;
	uint32_t MeshCount = 0;
	uint32_t UniqueIndexCount = 0;
	uint32_t PrimitiveIndexCount = 0;
	bool HasNormals = false;
	bool HasTangents = false;
	bool HasBitangents = false;
	bool HasTexcoords = false;
	std::vector<float3> Positions;	
	std::vector<float3> Normals;
	std::vector<float4> Tangents;
	std::vector<float3> Bitangents;
	std::vector<float2> Texcoords;
	tpr::RenderFormat IndexFormat = tpr::RenderFormat::UNKNOWN;
	std::vector<uint8_t> Indices;
	std::vector<MeshAsset_s> Meshes;
	std::vector<uint8_t> UniqueVertexIndices;
	std::vector<uint32_t> PrimitiveIndices; // 10 : 10 : 10

	std::string SourcePath;
};

ModelAsset_s* LoadModel(const std::wstring& Path);
