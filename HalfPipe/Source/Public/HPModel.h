#pragma once

#include <FileUtils/PathUtils.h>
#include <Render/RenderTypes.h>
#include <SurfMath.h>
#include <vector>

enum class HPModelVersion_e
{
	INITIAL = 0,

	CURRENT = INITIAL,
};

enum class FileStreamMode_e;
struct FileStream_s;

struct HPModel_s
{
	struct Mesh_s
	{
		uint32_t IndexOffset;
		uint32_t IndexCount;

		uint32_t MeshletOffset;
		uint32_t MeshletCount;

		std::wstring LibMaterialName;

		bool Serialize(FileStream_s& Stream);
	};

	struct Meshlet_s
	{
		uint32_t VertCount;
		uint32_t VertOffset;
		uint32_t PrimCount;
		uint32_t PrimOffset;
	};

	HPModelVersion_e Version = HPModelVersion_e::CURRENT;

	uint32_t VertexCount = 0;
	uint32_t IndexCount = 0;
	rl::RenderFormat IndexFormat = rl::RenderFormat::UNKNOWN;
	bool HasNormals = false;
	bool HasTangents = false;
	bool HasBitangents = false;
	bool HasTexcoords = false;
	std::vector<float3> Positions;
	std::vector<float3> Normals;
	std::vector<float4> Tangents;
	std::vector<float3> Bitangents;
	std::vector<float2> Texcoords;
	std::vector<uint8_t> Indices;
	std::vector<Mesh_s> Meshes;
	std::vector<Meshlet_s> Meshlets;
	std::vector<uint8_t> UniqueVertexIndices;
	std::vector<uint32_t> PrimitiveIndices; // 10 : 10 : 10

	std::wstring MaterialLibPath;
	std::wstring SourcePath;

	bool Serialize(const std::wstring& Path, FileStreamMode_e Mode);
	bool Serialize(FileStream_s& Stream);
};