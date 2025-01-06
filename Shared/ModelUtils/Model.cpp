#include "Model.h"
#include "FileUtils/PathUtils.h"
#include "FileUtils/FileStream.h"
#include "Logging/Logging.h"
#include "ModelUtils/ModelLoader.h"

#include <map>

struct ModelGlobals
{
	std::map<std::wstring, std::unique_ptr<ModelAsset_s>> LoadedModelAssets;
} G;

bool StreamModelAsset(const std::wstring& Path, FileStreamMode_e Mode, ModelAsset_s& Asset)
{
	const bool Writing = Mode == FileStreamMode_e::WRITE;

	FileStream_s Stream(Path, Mode);

	if (!Stream.IsOpen())
	{
		return false;
	}

	Stream.Stream(&Asset.VertexCount);
	Stream.Stream(&Asset.IndexCount);
	Stream.Stream(&Asset.MeshCount);
	Stream.Stream(&Asset.UniqueIndexCount);
	Stream.Stream(&Asset.PrimitiveIndexCount);
	Stream.Stream(&Asset.HasNormals);
	Stream.Stream(&Asset.HasTangents);
	Stream.Stream(&Asset.HasBitangents);
	Stream.Stream(&Asset.HasTexcoords);

	if (Writing)
		Asset.Positions.resize(Asset.VertexCount);

	Stream.StreamArray(Asset.Positions.data(), Asset.VertexCount);

	if (Asset.HasNormals)
	{
		if (Writing)
			Asset.Normals.resize(Asset.VertexCount);

		Stream.StreamArray(Asset.Normals.data(), Asset.VertexCount);
	}

	if (Asset.HasTangents)
	{
		if (Writing)
			Asset.Tangents.resize(Asset.VertexCount);

		Stream.StreamArray(Asset.Tangents.data(), Asset.VertexCount);
	}

	if (Asset.HasBitangents)
	{
		if (Writing)
			Asset.Bitangents.resize(Asset.VertexCount);

		Stream.StreamArray(Asset.Bitangents.data(), Asset.VertexCount);
	}

	if (Asset.HasTexcoords)
	{
		if (Writing)
			Asset.Texcoords.resize(Asset.VertexCount);

		Stream.StreamArray(Asset.Texcoords.data(), Asset.VertexCount);
	}

	Stream.Stream(&Asset.IndexFormat);

	size_t IndexBufByteCount = Asset.IndexFormat == tpr::RenderFormat::R32_UINT ? 4 * Asset.IndexCount : 2 * Asset.IndexCount;

	if(Writing)
		Asset.Indices.resize(IndexBufByteCount);

	Stream.StreamArray(Asset.Indices.data(), IndexBufByteCount);

	if (Writing)
		Asset.Meshes.resize(Asset.MeshCount);

	for (uint32_t MeshIt = 0; MeshIt < Asset.MeshCount; MeshIt++)
	{
		MeshAsset_s& Mesh = Asset.Meshes[MeshIt];

		Stream.StreamArray(Mesh.MaterialPath, PathUtils::MaxPath);
		Stream.Stream(&Mesh.IndexOffset);
		Stream.Stream(&Mesh.IndexCount);
		Stream.Stream(&Mesh.MeshletCount);

		if (Mesh.MeshletCount)
		{
			if(Writing)
				Mesh.Meshlets.resize(Mesh.MeshletCount);

			Stream.StreamArray(Mesh.Meshlets.data(), Mesh.MeshletCount);
		}
	}

	return true;
}

ModelAsset_s* LoadModel(const std::wstring& Path)
{
	std::wstring CookedPath = ReplacePathExtension(Path, L"rmdl");

	auto FoundIt = G.LoadedModelAssets.find(CookedPath);
	if (FoundIt != G.LoadedModelAssets.end())
	{
		return FoundIt->second.get();
	}

	ModelAsset_s* Asset = new ModelAsset_s;

	G.LoadedModelAssets[CookedPath] = std::unique_ptr<ModelAsset_s>(Asset);

	if (HasPathExtension(Path, L".rmdl"))
	{
		if (!StreamModelAsset(CookedPath, FileStreamMode_e::READ, *Asset))
		{
			G.LoadedModelAssets[CookedPath] = nullptr;
			return nullptr;
		}
	}
	else if (HasPathExtension(Path, L".obj"))
	{
		if (!LoadModelFromWavefront(Path.c_str(), *Asset))
		{
			G.LoadedModelAssets[CookedPath] = nullptr;
			return nullptr;
		}

		// Write cooked asset to disk

		StreamModelAsset(CookedPath, FileStreamMode_e::WRITE, *Asset);

		return Asset;
	}

	LOGERROR("Unsupported file format %S", GetPathExtension(Path).c_str());
	return nullptr;
}
