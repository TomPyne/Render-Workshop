#include "Model.h"

#include "Assets/Assets.h"
#include "FileUtils/PathUtils.h"
#include "FileUtils/FileStream.h"
#include "Logging/Logging.h"
#include "ModelUtils/ModelLoader.h"

AssetLibrary_s<ModelAsset_s> GModelAssets;

bool StreamModelAsset(const std::wstring& Path, FileStreamMode_e Mode, ModelAsset_s& Asset)
{
	FileStream_s Stream(Path, Mode);

	if (!Stream.IsOpen())
	{
		return false;
	}

	Stream.Stream(&Asset.VertexCount);
	Stream.Stream(&Asset.IndexCount);
	Stream.Stream(&Asset.HasNormals);
	Stream.Stream(&Asset.HasTangents);
	Stream.Stream(&Asset.HasBitangents);
	Stream.Stream(&Asset.HasTexcoords);
	Stream.Stream(&Asset.IndexFormat);

	Stream.Stream(&Asset.Positions, Asset.VertexCount);

	if (Asset.HasNormals)
	{
		Stream.Stream(&Asset.Normals, Asset.VertexCount);
	}

	if (Asset.HasTangents)
	{
		Stream.Stream(&Asset.Tangents, Asset.VertexCount);
	}

	if (Asset.HasBitangents)
	{
		Stream.Stream(&Asset.Bitangents, Asset.VertexCount);
	}

	if (Asset.HasTexcoords)
	{
		Stream.Stream(&Asset.Texcoords, Asset.VertexCount);
	}	

	size_t IndexBufByteCount = Asset.IndexFormat == tpr::RenderFormat::R32_UINT ? 4 * Asset.IndexCount : 2 * Asset.IndexCount;

	Stream.Stream(&Asset.Indices, IndexBufByteCount);

	Stream.Stream(&Asset.Meshes);
	Stream.Stream(&Asset.Meshlets);
	Stream.Stream(&Asset.UniqueVertexIndices);
	Stream.Stream(&Asset.PrimitiveIndices);

	return true;
}

ModelAsset_s* LoadModel(const std::wstring& Path)
{
	std::wstring CookedPath = ReplacePathExtension(Path, L"rmdl");

	if (ModelAsset_s* Found = GModelAssets.FindAsset(CookedPath))
	{
		return Found;
	}

	ModelAsset_s* Asset = new ModelAsset_s;

	if (!GAssetConfig.SkipCookedLoading)
	{
		if (StreamModelAsset(CookedPath, FileStreamMode_e::READ, *Asset))
		{
			return GModelAssets.CreateAsset(CookedPath, Asset);
		}
	}

	if (HasPathExtension(Path, L".obj"))
	{
		if (LoadModelFromWavefront(Path.c_str(), *Asset))
		{
			if (!GAssetConfig.SkipCookedWriting)
			{
				StreamModelAsset(CookedPath, FileStreamMode_e::WRITE, *Asset);
			}

			return GModelAssets.CreateAsset(CookedPath, Asset);;
		}
	}

	LOGERROR("Unsupported file format %S", GetPathExtension(Path).c_str());
	return nullptr;
}
