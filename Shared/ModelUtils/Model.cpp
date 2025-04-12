#include "Model.h"

#include "Assets/Assets.h"
#include "FileUtils/PathUtils.h"
#include "FileUtils/FileStream.h"
#include "Logging/Logging.h"
#include "ModelUtils/ModelLoader.h"

#include <filesystem>

#if 0
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

void MakeDirectoryForCookedAsset(const std::wstring& CookedPath)
{
	std::filesystem::path ParentPath = std::filesystem::path(CookedPath).parent_path();
	std::filesystem::create_directories(ParentPath);
}

bool IsSupportedSourceType(const std::wstring& ext)
{
	if (ext == L".obj")
	{
		return true;
	}

	return false;
}

std::wstring GetAssetPath()
{
	return L"Assets/";
}

std::wstring GetCookedPath()
{
	return L"Cooked/";
}

bool IsAssetPath(const std::wstring& Path)
{
	return Path.starts_with(GetAssetPath());
}

bool IsCookedPath(const std::wstring& Path)
{
	return Path.starts_with(GetCookedPath());
}

std::wstring AssetPathToCookedPath(const std::wstring& AssetPath, const std::wstring& CookedExtension)
{
	std::wstring CookedAssetPath = GetCookedPath() + AssetPath.substr(GetAssetPath().size());
	return ReplacePathExtension(CookedAssetPath, CookedExtension.c_str());
}

std::wstring GetModelCookedPath(const std::wstring& AssetPath)
{
	return AssetPathToCookedPath(AssetPath, L"hp_mdl");
}

bool CookModel(const std::wstring& AssetPath, ModelAsset_s** OutAsset)
{
	if (!IsAssetPath(AssetPath))
	{		
		return FAILMSG("%S needs to be in the Assets/ dir", AssetPath.c_str());
	}

	if (!IsSupportedSourceType(GetPathExtension(AssetPath)))
	{
		return FAILMSG("%S is not a supported source model extension", AssetPath.c_str());
	}

	std::wstring CookedAssetPath = GetModelCookedPath(AssetPath);

	MakeDirectoryForCookedAsset(CookedAssetPath);

	LOGINFO("Cooking %S to %S", AssetPath.c_str(), CookedAssetPath.c_str());

	ModelAsset_s* Asset = new ModelAsset_s;

	if (HasPathExtension(AssetPath, L".obj"))
	{
		if (LoadModelFromWavefront(AssetPath.c_str(), *Asset))
		{
			if (!GAssetConfig.SkipCookedWriting)
			{
				StreamModelAsset(CookedAssetPath, FileStreamMode_e::WRITE, *Asset);
			}
		}
	}

	if (OutAsset)
	{
		*OutAsset = Asset;
	}
	else
	{
		delete Asset;
	}

	return true;
}

ModelAsset_s* LoadModel(const std::wstring& Path)
{
	if (IsAssetPath(Path))
	{
		std::wstring CookedPath = GetModelCookedPath(Path);
		if (ModelAsset_s* Found = GModelAssets.FindAsset(CookedPath))
		{
			return Found;
		}

		if (!GAssetConfig.CookOnDemand)
		{
			LOGERROR("Can't load asset %S when CookOnDemand is disabled", Path.c_str());
			return nullptr;
		}
		else
		{
			ModelAsset_s* Asset = nullptr;
			
			if (!CookModel(Path, &Asset))
			{
				return nullptr;
			}
			else
			{
				CHECK(Asset != nullptr);

				return GModelAssets.CreateAsset(CookedPath, Asset);
			}
		}
	}
	else if (IsCookedPath(Path))
	{		
		if (ModelAsset_s* Found = GModelAssets.FindAsset(Path))
		{
			return Found;
		}

		ModelAsset_s* Asset = new ModelAsset_s;

		if (StreamModelAsset(Path, FileStreamMode_e::READ, *Asset))
		{
			return GModelAssets.CreateAsset(Path, Asset);
		}

		return GModelAssets.CreateAsset(Path, Asset);
	}

	LOGERROR("Could not find asset %S to load or cook", Path.c_str());
	return nullptr;
}
#endif