#include "Assets/MeshManager.h"

#include "Assets/MaterialManager.h"
#include "Rendering/Materials.h"
#include "Rendering/Mesh.h"

#include <HalfPipe/Source/Public/GltfReader.h>
#include <HalfPipe/Source/Public/WaveFrontReader.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/JsonHelpers.h>
#include <Shared/Logging/Logging.h>
#include <Shared/ModelUtils/TangentSpace.h>
#include <Shared/StringUtils/StringUtils.h>
#include <Render/Render.h>
#include <SurfMath.h>

#include <unordered_map>

#define MESH_ASSET_VERSION_INITIAL 1
#define MESH_ASSET_VERSION_CURRENT MESH_ASSET_VERSION_INITIAL

static_assert(kMeshMaxTexcoords == kTangentSpaceMaxTexcoords, "Mesh and tangent space texcoord limits must match");
static_assert(kMeshMaxTexcoords == GltfReader_c::MaxTexcoords, "Mesh and glTF reader texcoord limits must match");

namespace MeshManager
{

struct MeshManagerGlobals_s
{
	// Content addressable storage of loaded meshes.
	std::unordered_map<uint64_t, std::shared_ptr<Mesh_s>> LoadedMeshes;
} G;

// Geometry from a source file reader, already converted into engine space.
struct SourceMesh_s
{
	TangentSpaceStream_s Positions;
	TangentSpaceStream_s Normals;
	TangentSpaceStream_s Texcoords[kMeshMaxTexcoords];
	uint32_t TexcoordCount = 0;
	uint32_t VertexCount = 0;

	const std::vector<uint32_t>* Indices = nullptr;
	// Material slot of each triangle, indexing SlotNames.
	const std::vector<uint32_t>* Attributes = nullptr;
	std::vector<std::wstring> SlotNames;

	// Set when the conversion into engine space mirrored the geometry.
	bool ReverseWinding = false;
};

std::shared_ptr<Mesh_s> BuildMesh(const JsonValue_s& Data, const Path_s& Path, const SourceMesh_s& Source)
{
	std::shared_ptr<Mesh_s> NewMesh = std::make_shared<Mesh_s>();

	const std::vector<uint32_t>& Indices = *Source.Indices;
	const std::vector<uint32_t>& Attributes = *Source.Attributes;

	std::vector<std::vector<uint32_t>> SurfaceIndices;

	for (uint32_t AttrIt = 0; AttrIt < Attributes.size(); AttrIt++)
	{
		uint32_t IndexOffset = AttrIt * 3;
		uint32_t Attribute = Attributes[AttrIt];
		if (Attribute >= SurfaceIndices.size())
		{
			SurfaceIndices.resize(Attribute + 1);
		}

		SurfaceIndices[Attribute].push_back(Indices[IndexOffset + 0]);
		SurfaceIndices[Attribute].push_back(Indices[IndexOffset + (Source.ReverseWinding ? 2 : 1)]);
		SurfaceIndices[Attribute].push_back(Indices[IndexOffset + (Source.ReverseWinding ? 1 : 2)]);
	}

	// The readers reserve slot 0 for a synthetic "default" material that exports never
	// reference, so drop any slot that ended up with no faces and keep the material name of the
	// slots that survive, which is what MaterialSlot binds against below.
	std::vector<std::wstring> SurfaceNames;
	{
		std::vector<std::vector<uint32_t>> UsedSurfaceIndices;
		for (uint32_t SlotIt = 0; SlotIt < SurfaceIndices.size(); SlotIt++)
		{
			if (SurfaceIndices[SlotIt].empty())
			{
				continue;
			}

			UsedSurfaceIndices.push_back(std::move(SurfaceIndices[SlotIt]));
			SurfaceNames.push_back(SlotIt < Source.SlotNames.size() ? Source.SlotNames[SlotIt] : std::wstring{});
		}

		SurfaceIndices = std::move(UsedSurfaceIndices);
	}

	std::vector<uint32_t> SourceIndices;
	SourceIndices.reserve(Indices.size());
	for (const std::vector<uint32_t>& Surface : SurfaceIndices)
	{
		SourceIndices.insert(SourceIndices.end(), Surface.begin(), Surface.end());
	}

	TangentSpaceInput_s TangentInput = {};
	TangentInput.Positions = Source.Positions;
	TangentInput.Normals = Source.Normals;
	for (uint32_t Channel = 0; Channel < Source.TexcoordCount; Channel++)
	{
		TangentInput.Texcoords[Channel] = Source.Texcoords[Channel];
	}
	TangentInput.TexcoordCount = Source.TexcoordCount;
	TangentInput.VertexCount = Source.VertexCount;
	TangentInput.Indices = SourceIndices.data();
	TangentInput.IndexCount = static_cast<uint32_t>(SourceIndices.size());

	TangentSpaceOutput_s TangentOutput;
	if (!GenerateTangents(TangentInput, TangentOutput))
	{
		LOGWARNING("[MeshManager::RequestMesh] Failed to generate tangents for mesh: %s", Path.ToString().c_str());
		return nullptr;
	}

	NewMesh->Vertices = std::move(TangentOutput.Positions);
	NewMesh->Indices = std::move(TangentOutput.Indices);

	for (const float3& Vertex : NewMesh->Vertices)
	{
		NewMesh->Bounds.Grow(Vertex);
	}

	const uint32_t VertexCount = static_cast<uint32_t>(NewMesh->Vertices.size());

	NewMesh->PositionBuffer = rl::CreateStructuredBuffer(NewMesh->Vertices.data(), VertexCount);
	NewMesh->NormalBuffer = rl::CreateStructuredBuffer(TangentOutput.Normals.data(), VertexCount);
	NewMesh->TangentBuffer = rl::CreateStructuredBuffer(TangentOutput.Tangents.data(), VertexCount);

	NewMesh->PositionBufferSRV = rl::CreateStructuredBufferSRV(NewMesh->PositionBuffer, 0u, VertexCount, static_cast<uint32_t>(sizeof(float3)));
	NewMesh->NormalBufferSRV = rl::CreateStructuredBufferSRV(NewMesh->NormalBuffer, 0u, VertexCount, static_cast<uint32_t>(sizeof(float3)));
	NewMesh->TangentBufferSRV = rl::CreateStructuredBufferSRV(NewMesh->TangentBuffer, 0u, VertexCount, static_cast<uint32_t>(sizeof(float4)));

	for (uint32_t Channel = 0; Channel < Source.TexcoordCount; Channel++)
	{
		NewMesh->TexcoordBuffers[Channel] = rl::CreateStructuredBuffer(TangentOutput.Texcoords[Channel].data(), VertexCount);
		NewMesh->TexcoordBufferSRVs[Channel] = rl::CreateStructuredBufferSRV(NewMesh->TexcoordBuffers[Channel], 0u, VertexCount, static_cast<uint32_t>(sizeof(float2)));
	}

	NewMesh->IndexBuffer = rl::CreateIndexBufferFromArray(NewMesh->Indices.data(), NewMesh->Indices.size());

	MeshUniformData_s MeshUniformData = {};
	MeshUniformData.PositionBufferIndex = rl::GetDescriptorIndex(NewMesh->PositionBufferSRV);
	MeshUniformData.NormalBufferIndex = rl::GetDescriptorIndex(NewMesh->NormalBufferSRV);
	MeshUniformData.TangentBufferIndex = rl::GetDescriptorIndex(NewMesh->TangentBufferSRV);
	for (uint32_t Channel = 0; Channel < Source.TexcoordCount; Channel++)
	{
		MeshUniformData.TexcoordBufferIndices[Channel] = rl::GetDescriptorIndex(NewMesh->TexcoordBufferSRVs[Channel]);
	}
	NewMesh->MeshUniforms = rl::CreateConstantBuffer(&MeshUniformData);

	std::vector<std::shared_ptr<MaterialShaderInstance_c>> Materials;
	Materials.resize(SurfaceIndices.size());
	auto MaterialsIt = Data.Json.find("Materials");
	if (MaterialsIt != Data.Json.end())
	{
		if (MaterialsIt->is_array())
		{
			uint32_t EntryIt = 0;
			for (const Json_t& MaterialNode : *MaterialsIt)
			{
				const uint32_t EntryIndex = EntryIt++;

				Path_s MaterialAssetPath;
				if (!JsonHelpers::ParsePath(MaterialNode, "MaterialAssetPath", MaterialAssetPath))
				{
					continue;
				}

				// MaterialSlot binds to the material name it matches, so reordering the slots in
				// blender cannot silently swap two materials over. Entries without one fall back
				// to the order they are authored in.
				uint32_t Slot = EntryIndex;
				std::string MaterialSlot;
				if (JsonHelpers::ParseString(MaterialNode, "MaterialSlot", MaterialSlot))
				{
					const std::wstring WideMaterialSlot = NarrowToWide(MaterialSlot);

					bool FoundSlot = false;
					for (uint32_t SlotIt = 0; SlotIt < SurfaceNames.size(); SlotIt++)
					{
						if (SurfaceNames[SlotIt] == WideMaterialSlot)
						{
							Slot = SlotIt;
							FoundSlot = true;
							break;
						}
					}

					if (!FoundSlot)
					{
						LOGWARNING("[MeshManager::RequestMesh] No material slot named '%s' in mesh: %s", MaterialSlot.c_str(), Path.ToString().c_str());
						continue;
					}
				}

				if (Slot >= Materials.size())
				{
					LOGWARNING("[MeshManager::RequestMesh] Material entry %u has no matching surface in mesh: %s", EntryIndex, Path.ToString().c_str());
					continue;
				}

				Materials[Slot] = MaterialManager::RequestMaterialInstance(MaterialAssetPath);
			}
		}
		else
		{
			LOGWARNING("[MeshManager::RequestMesh] Materials field is not an array");
		}
	}

	for (uint32_t SlotIt = 0; SlotIt < Materials.size(); SlotIt++)
	{
		CLOGWARNING(Materials[SlotIt] == nullptr, "[MeshManager::RequestMesh] Surface '%S' has no material and will not draw: %s", SurfaceNames[SlotIt].c_str(), Path.ToString().c_str());
	}

	uint32_t CurrentIndexOffset = 0;
	for (size_t SurfaceIt = 0; SurfaceIt < SurfaceIndices.size(); SurfaceIt++)
	{
		Surface_s NewSurface = {};
		NewSurface.Material = Materials[SurfaceIt];
		NewSurface.IndexOffset = CurrentIndexOffset;
		NewSurface.IndexCount = static_cast<uint32_t>(SurfaceIndices[SurfaceIt].size());

		for (uint32_t IndexIt = NewSurface.IndexOffset; IndexIt < NewSurface.IndexOffset + NewSurface.IndexCount; IndexIt++)
		{
			NewSurface.Bounds.Grow(NewMesh->Vertices[NewMesh->Indices[IndexIt]]);
		}

		CurrentIndexOffset += NewSurface.IndexCount;
		NewMesh->Surfaces.push_back(NewSurface);
	}

	G.LoadedMeshes[Data.GetHash()] = NewMesh;

	NewMesh->Ready = true;
	return NewMesh;
}

std::shared_ptr<Mesh_s> RequestMeshObj(const JsonValue_s& Data, const Path_s& Path)
{
	WaveFrontReader_c Reader;
	if (!Reader.Load(Path.ToWString().c_str()))
	{
		LOGWARNING("[MeshManager::RequestMesh] Failed to load mesh: %s", Path.ToString().c_str());
		return nullptr;
	}

	if (Reader.Vertices.empty() || Reader.Indices.empty())
	{
		LOGWARNING("[MeshManager::RequestMesh] Mesh has no geometry: %s", Path.ToString().c_str());
		return nullptr;
	}

	// Blender exports right handed OBJ, so mirror X into our left handed space. The winding is
	// reversed when building to match, otherwise the mirror would flip which faces get culled.
	for (WaveFrontReader_c::Vertex_s& Vertex : Reader.Vertices)
	{
		Vertex.Position.x = -Vertex.Position.x;
		Vertex.Normal.x = -Vertex.Normal.x;
		Vertex.Texcoord.y = 1.0f - Vertex.Texcoord.y;
	}

	SourceMesh_s Source;
	Source.Positions = { &Reader.Vertices[0].Position, sizeof(WaveFrontReader_c::Vertex_s) };
	Source.Normals = { &Reader.Vertices[0].Normal, sizeof(WaveFrontReader_c::Vertex_s) };
	Source.Texcoords[0] = { &Reader.Vertices[0].Texcoord, sizeof(WaveFrontReader_c::Vertex_s) };
	Source.TexcoordCount = 1;
	Source.VertexCount = static_cast<uint32_t>(Reader.Vertices.size());
	Source.Indices = &Reader.Indices;
	Source.Attributes = &Reader.Attributes;
	Source.ReverseWinding = true;

	for (const WaveFrontReader_c::Material_s& Material : Reader.Materials)
	{
		Source.SlotNames.push_back(Material.Name);
	}

	return BuildMesh(Data, Path, Source);
}

std::shared_ptr<Mesh_s> RequestMeshGlb(const JsonValue_s& Data, const Path_s& Path)
{
	GltfReader_c Reader;
	if (!Reader.Load(Path.ToWString().c_str()))
	{
		LOGWARNING("[MeshManager::RequestMesh] Failed to load mesh: %s", Path.ToString().c_str());
		return nullptr;
	}

	if (Reader.Vertices.empty() || Reader.Indices.empty())
	{
		LOGWARNING("[MeshManager::RequestMesh] Mesh has no geometry: %s", Path.ToString().c_str());
		return nullptr;
	}

	if (Reader.TexcoordCount == 0)
	{
		LOGWARNING("[MeshManager::RequestMesh] Mesh needs TEXCOORD_0 to build tangents: %s", Path.ToString().c_str());
		return nullptr;
	}

	// The Unreal to FBX to Blender round trip leaves the glb right handed, and with Blender's FBX
	// import axes the node transform lands the mirror on Z, so flip Z into our left handed space.
	// The winding is reversed when building to match, otherwise the mirror would flip which faces
	// get culled. glTF already uses a top left UV origin, so unlike the OBJ path the UVs are not flipped.
	for (GltfReader_c::Vertex_s& Vertex : Reader.Vertices)
	{
		Vertex.Position.z = -Vertex.Position.z;
		Vertex.Normal.z = -Vertex.Normal.z;
	}

	SourceMesh_s Source;
	Source.Positions = { &Reader.Vertices[0].Position, sizeof(GltfReader_c::Vertex_s) };
	Source.Normals = { &Reader.Vertices[0].Normal, sizeof(GltfReader_c::Vertex_s) };
	for (uint32_t Channel = 0; Channel < Reader.TexcoordCount; Channel++)
	{
		Source.Texcoords[Channel] = { &Reader.Vertices[0].Texcoords[Channel], sizeof(GltfReader_c::Vertex_s) };
	}
	Source.TexcoordCount = Reader.TexcoordCount;
	Source.VertexCount = static_cast<uint32_t>(Reader.Vertices.size());
	Source.Indices = &Reader.Indices;
	Source.Attributes = &Reader.Attributes;
	Source.SlotNames = Reader.MaterialNames;
	Source.ReverseWinding = true;

	return BuildMesh(Data, Path, Source);
}

std::shared_ptr<Mesh_s> RequestErrorMesh()
{
	Path_s ErrorMeshPath = Path_s(PathDirectory_e::Assets, L"Game", L"Meshes/ErrorText.hp_mdl");
	return RequestMesh(ErrorMeshPath, false);
}

std::shared_ptr<Mesh_s> RequestMesh(const Path_s& Path, bool ErrorMeshIfMissing)
{
	Json_t Json;
	const bool JsonLoaded = LoadJsonFromFile(Path.ToWString(), Json);
	if (!JsonLoaded)
	{
		LOGWARNING("[MeshManager::RequestMesh] Failed to load Mesh json from path %S", Path.ToWString().c_str());
		return ErrorMeshIfMissing ? RequestErrorMesh() : nullptr;
	}

	return RequestMesh(Json, ErrorMeshIfMissing);
}

std::shared_ptr<Mesh_s> RequestMesh(const JsonValue_s& Data, bool ErrorMeshIfMissing)
{
	// Compute hash of data to use as a key for caching
	auto It = G.LoadedMeshes.find(Data.GetHash());
	if (It != G.LoadedMeshes.end())
	{
		return It->second;
	}

	int32_t Version = -1;
	JsonHelpers::ParseInt(Data, "Version", Version);
	if (Version != MESH_ASSET_VERSION_CURRENT)
	{
		LOGWARNING("[MeshManager::RequestMesh] Unsupported mesh asset version: %d", Version);
		return ErrorMeshIfMissing ? RequestErrorMesh() : nullptr;
	}

	std::string FileFormat;
	if (!JsonHelpers::ParseString(Data, "SourceFileType", FileFormat))
	{
		LOGWARNING("[MeshManager::RequestMesh] Missing FileFormat field");
		return ErrorMeshIfMissing ? RequestErrorMesh() : nullptr;
	}

	if (FileFormat != "obj" && FileFormat != "glb")
	{
		LOGWARNING("[MeshManager::RequestMesh] Unsupported file format for mesh: %s", FileFormat.c_str());
		return ErrorMeshIfMissing ? RequestErrorMesh() : nullptr;
	}

	Path_s Path;
	if (!ENSUREMSG(JsonHelpers::ParsePath(Data, "SourceFilePath", Path), "[MeshManager::RequestMesh] Missing SourceFile field"))
	{
		return nullptr;
	}

	LOGINFO("[MeshManager::RequestMesh] Building mesh: %s", Path.ToString().c_str());

	return FileFormat == "obj" ? RequestMeshObj(Data, Path) : RequestMeshGlb(Data, Path);
}

}