#include "Materials.h"

#include "Assets/Assets.h"
#include "FileUtils/FileStream.h"
#include "FileUtils/PathUtils.h"
#include "Logging/Logging.h"

#include <map>
#include <shared_mutex>

AssetLibrary_s<MaterialAsset_s> GMaterialsAssets;

MaterialAsset_s* GetDefaultMaterialAsset()
{
    static const std::wstring DefaultMaterialAssetPath = L"DefaultMaterial";

    if (MaterialAsset_s* Found = GMaterialsAssets.FindAsset(DefaultMaterialAssetPath))
    {
        return Found;
    }

    MaterialAsset_s* DefaultMat = GMaterialsAssets.CreateAsset(DefaultMaterialAssetPath);

    DefaultMat->Albedo = float3(0.8f, 0.2f, 0.2f);
    DefaultMat->SourcePath = DefaultMaterialAssetPath;

    return DefaultMat;
}

bool StreamMaterialAsset(const std::wstring& FilePath, FileStreamMode_e Mode, MaterialAsset_s& Asset)
{
    FileStream_s File = FileStream_s(FilePath, Mode);

    if (!File.IsOpen())
    {
        return false;
    }

    File.Stream(&Asset.Albedo);
    File.Stream(&Asset.Metallic);
    File.Stream(&Asset.Roughness);
    File.StreamStr(&Asset.AlbedoTexture);
    File.StreamStr(&Asset.NormalTexture);
    File.StreamStr(&Asset.MetallicTexture);
    File.StreamStr(&Asset.RoughnessTexture);

    return true;

}

MaterialAsset_s* LoadMaterialAsset(const std::wstring& FilePath)
{
    MaterialAsset_s* Asset = nullptr;

    if (MaterialAsset_s* Found = GMaterialsAssets.FindAsset(FilePath))
    {
        return Found;
    }

    if (!Asset)
    {
        Asset = new MaterialAsset_s;        

        if (HasPathExtension(FilePath, L".rmat")) // Load from asset
        {
            if (StreamMaterialAsset(FilePath, FileStreamMode_e::READ, *Asset))
            {
                GMaterialsAssets.CreateAsset(FilePath, Asset);
            }
            else
            {
                delete Asset;

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

void WriteMaterialAsset(const std::wstring& FileName, MaterialAsset_s* Asset)
{
    if (GAssetConfig.SkipCookedWriting)
        return;

    if (!Asset)
        return;

    std::wstring Path = ReplacePathExtension(FileName, L"rmat");

    StreamMaterialAsset(FileName, FileStreamMode_e::WRITE, *Asset);
}
