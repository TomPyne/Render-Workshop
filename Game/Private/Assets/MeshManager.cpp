#include "Assets/MeshManager.h"

#include "Assets/MaterialManager.h"
#include "Rendering/Materials.h"
#include "Rendering/Mesh.h"

#include <HalfPipe/Source/Public/WaveFrontReader.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/JsonHelpers.h>
#include <Shared/Logging/Logging.h>
#include <Render/Render.h>
#include <SurfMath.h>

#include <unordered_map>

#define MESH_ASSET_VERSION_INITIAL 1
#define MESH_ASSET_VERSION_CURRENT MESH_ASSET_VERSION_INITIAL

namespace MeshManager
{

struct MeshManagerGlobals_s
{
	// Content addressable storage of loaded meshes.
	std::unordered_map<uint64_t, std::shared_ptr<Mesh_s>> LoadedMeshes;
} G;

std::shared_ptr<Mesh_s> RequestMeshObj(const JsonValue_s& Data)
{
	// Compute hash of data to use as a key for caching
	auto It = G.LoadedMeshes.find(Data.GetHash());
	if (It != G.LoadedMeshes.end())
	{
		return It->second;
	}

	std::wstring Path;
	if (!ENSUREMSG(JsonHelpers::ParseWString(Data, "SourceFilePath", Path), "[MeshManager::RequestMesh] Missing SourceFile field"))
	{
		return nullptr;
	}

	Path_s FullPath = Path_s(Path);

	LOGINFO("[MeshManager::RequestMesh] Building mesh: %s", FullPath.ToString().c_str());

	std::shared_ptr<Mesh_s> NewMesh = std::make_shared<Mesh_s>();	

	WaveFrontReader_c Reader;
	if (!Reader.Load(FullPath.ToWString().c_str()))
	{
		LOGWARNING("[MeshManager::RequestMesh] Failed to load mesh: %s", FullPath.ToString().c_str());
		return nullptr;
	}

	std::vector<float3> Positions;
	std::vector<std::vector<uint32_t>> SurfaceIndices;

	Positions.resize(Reader.Vertices.size());
	for (uint32_t VertIt = 0; VertIt < Reader.Vertices.size(); VertIt++)
	{
		Positions[VertIt] = Reader.Vertices[VertIt].Position;
	}

	for (uint32_t AttrIt = 0; AttrIt < Reader.Attributes.size(); AttrIt++)
	{
		uint32_t IndexOffset = AttrIt * 3;
		uint32_t Attribute = Reader.Attributes[AttrIt];
		if (Attribute >= SurfaceIndices.size())
		{
			SurfaceIndices.resize(Attribute + 1);
		}

		SurfaceIndices[Attribute].push_back(Reader.Indices[IndexOffset + 0]);
		SurfaceIndices[Attribute].push_back(Reader.Indices[IndexOffset + 1]);
		SurfaceIndices[Attribute].push_back(Reader.Indices[IndexOffset + 2]);
	}

	std::vector<uint32_t> CombinedIndices;
	for (const std::vector<uint32_t> Surface : SurfaceIndices)
	{
		CombinedIndices.insert(CombinedIndices.end(), Surface.begin(), Surface.end());
	}

	NewMesh->PositionBuffer = rl::CreateStructuredBuffer(Positions.data(), Positions.size());
	NewMesh->PositionBufferSRV = rl::CreateStructuredBufferSRV(NewMesh->PositionBuffer, 0u, static_cast<uint32_t>(Positions.size()), static_cast<uint32_t>(sizeof(float3)));
	NewMesh->IndexBuffer = rl::CreateIndexBufferFromArray(CombinedIndices.data(), CombinedIndices.size());

	MeshUniformData_s MeshUniformData = {};
	MeshUniformData.PositionBufferIndex = rl::GetDescriptorIndex(NewMesh->PositionBufferSRV);
	NewMesh->MeshUniforms = rl::CreateConstantBuffer(&MeshUniformData);

	std::vector<std::shared_ptr<Material_s>> Materials;
	Materials.reserve(SurfaceIndices.size());
	auto MaterialsIt = Data.Json.find("Materials");
	if (MaterialsIt != Data.Json.end())
	{
		if (MaterialsIt->is_array())
		{
			for (const Json_t& MaterialNode : *MaterialsIt)
			{
				std::wstring MaterialAssetPath;
				if (JsonHelpers::ParseWString(MaterialNode, "MaterialAssetPath", MaterialAssetPath))
				{
					Materials.push_back(MaterialManager::RequestMaterial(MaterialAssetPath));
				}
				else
				{
					Materials.push_back(nullptr);
				}
			}
		}
		else
		{
			LOGWARNING("[MeshManager::RequestMesh] Materials field is not an array");
		}
	}

	uint32_t CurrentIndexOffset = 0;
	for (const std::vector<uint32_t> Surface : SurfaceIndices)
	{
		Surface_s NewSurface = {};
		NewSurface.Material = nullptr; // Assign Materials later
		NewSurface.IndexOffset = CurrentIndexOffset;
		NewSurface.IndexCount = static_cast<uint32_t>(Surface.size());
		CurrentIndexOffset += NewSurface.IndexCount;
	}

	G.LoadedMeshes[Data.GetHash()] = NewMesh;

	NewMesh->Ready = true;
	return NewMesh;
}

std::shared_ptr<Mesh_s> RequestMesh(const Path_s& Path)
{
	Json_t Json;
	if (ENSUREMSG(LoadJsonFromFile(Path.ToWString(), Json), "[MeshManager::RequestMesh] Failed to load Mesh json from path %S", Path.ToWString().c_str()))
	{
		return RequestMesh(Json);
	}
	return nullptr;
}

std::shared_ptr<Mesh_s> RequestMesh(const JsonValue_s& Data)
{
	int32_t Version = -1;
	JsonHelpers::ParseInt(Data, "Version", Version);
	if (!ENSUREMSG(Version == MESH_ASSET_VERSION_CURRENT,"[MeshManager::RequestMesh] Unsupported mesh asset version: %d", Version))
	{
		return nullptr;
	}

	std::string FileFormat;
	if (!ENSUREMSG(JsonHelpers::ParseString(Data, "SourceFileType", FileFormat), "[MeshManager::RequestMesh] Missing FileFormat field"))
	{
		return nullptr;
	}

	if (FileFormat == "obj")
	{
		return RequestMeshObj(Data);
	}
	else
	{
		LOGWARNING("[MeshManager::RequestMesh] Unsupported file format for mesh: %s", FileFormat.c_str());
		return nullptr;
	}
}

}