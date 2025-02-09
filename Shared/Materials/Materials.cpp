#include "Materials.h"

#include "Assets/AssetConfig.h"
#include "FileUtils/FileStream.h"
#include "FileUtils/PathUtils.h"
#include "Logging/Logging.h"

#include <fstream>
#include <map>

struct MaterialGlobals
{
	std::map<std::wstring, std::unique_ptr<MaterialAsset_s>> LoadedMaterialAssets;
} G;

MaterialAsset_s* GetDefaultMaterialAsset()
{
    static const std::wstring DefaultMaterialAssetPath = L"DefaultMaterial";

    auto Found = G.LoadedMaterialAssets.find(DefaultMaterialAssetPath);
    if (Found != G.LoadedMaterialAssets.end())
    {
        return Found->second.get();
    }

    MaterialAsset_s* DefaultMat = new MaterialAsset_s;
    G.LoadedMaterialAssets.emplace(DefaultMaterialAssetPath, DefaultMat);

    DefaultMat->Albedo = float3(0.8f, 0.2f, 0.2f);
    DefaultMat->SourcePath = DefaultMaterialAssetPath;

    return DefaultMat;
}

MaterialAsset_s* LoadMaterialAsset(const std::wstring& FilePath)
{
    MaterialAsset_s* Asset = nullptr;

    auto Found = G.LoadedMaterialAssets.find(FilePath);
    if (Found != G.LoadedMaterialAssets.end())
    {
        Asset = Found->second.get();
    }

    if (!Asset)
    {
        if (HasPathExtension(FilePath, L".rmat")) // Load from asset
        {
            IFileStream_s Stream(FilePath);
            if (Stream.IsOpen())
            {
                Asset = new MaterialAsset_s;
                G.LoadedMaterialAssets.emplace(FilePath, Asset);

                Stream.Read(&Asset->Albedo);
                Stream.ReadArray(&Asset->AlbedoTexturePath, PathUtils::MaxPath);

                Asset->SourcePath = FilePath;
            }
            else
            {
                LOGERROR("Invalid path, has the material been generated?");
                Asset = GetDefaultMaterialAsset();
            }
        }
        else
        {
            LOGERROR("Invalid file extension");
            Asset = GetDefaultMaterialAsset();
        }
    }

    return Asset;
}

void WriteMaterialAsset(const std::wstring& FileName, const MaterialAsset_s* const Asset)
{
    if (GAssetConfig.SkipCookedWriting)
        return;

    std::wstring Path = ReplacePathExtension(FileName, L"rmat");

    // Cache
    {
        OFileStream_s Stream(Path);
        if (ENSUREMSG(Stream.IsOpen(), "Failed to open material file for write %S", Path))
        {
            Stream.Write(&Asset->Albedo);
            Stream.WriteArray(Asset->AlbedoTexturePath, PathUtils::MaxPath);
        }
    }
}
