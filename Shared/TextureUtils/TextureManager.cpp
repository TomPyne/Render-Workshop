#include "TextureManager.h"

#include "Assets/Assets.h"
#include "DDSTextureLoader.h"
#include "FileUtils/FileStream.h"
#include "FileUtils/PathUtils.h"
#include "Logging/Logging.h"
#include "StringUtils/StringUtils.h"

#include <Render/Textures.h>
#include <Render/TextureInfo.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

// TODO:
// 1. Enqueue texture loads on a helper thread, return the Texture_t handle immediately.
// 2. Default textures

AssetLibrary_s<TextureAsset_s> GTextureAssets;

constexpr bool IsPowerOfTwo(int Num)
{
    if (Num <= 0)
    {
        return false;
    }

    if ((Num & (Num - 1)) == 0)
    {
        return true;
    }

    return false;
}

bool StreamRTex(const wchar_t* FilePath, FileStreamMode_e Mode, TextureAsset_s* Asset)
{
    CHECK(FilePath != nullptr);
    CHECK(Asset != nullptr);

    FileStream_s Stream(FilePath, Mode);

    if (!Stream.IsOpen())
    {
        return false;
    }

    Stream.Stream(&Asset->Version);
    Stream.Stream(&Asset->Width);
    Stream.Stream(&Asset->Height);
    Stream.Stream(&Asset->DepthOrArraySize);
    Stream.Stream(&Asset->Dimension);
    Stream.Stream(&Asset->Format);
    Stream.Stream(&Asset->MipCount);
    Stream.Stream(&Asset->Cubemap);
    Stream.Stream(&Asset->SubResourceInfos);
    Stream.Stream(&Asset->Data);
    Stream.Stream(&Asset->SourcePath);

    return true;
}

bool LoadRTexTexture(const wchar_t* FilePath, TextureAsset_s* Asset)
{
    return StreamRTex(FilePath, FileStreamMode_e::READ, Asset);
}

bool LoadSTBITexture(const char* FilePath, TextureAsset_s* Asset)
{
    CHECK(Asset != nullptr);

    int X, Y, Comp;
    void* TexData = stbi_load(FilePath, &X, &Y, &Comp, 4);

    if (!TexData)
    {
        return FAILMSG("Failed to load texture %s", FilePath);
    }

    Asset->Width = X;
    Asset->Height = Y;
    Asset->DepthOrArraySize = 1;
    Asset->Dimension = TextureAssetDimension_e::TWODIM;
    Asset->Format = tpr::RenderFormat::R8G8B8A8_UNORM;
    Asset->MipCount = 1;
    Asset->Cubemap = false;

    size_t NumBytes, RowBytes;
    tpr::GetTextureSurfaceInfo(Asset->Width, Asset->Height, Asset->Format, &NumBytes, &RowBytes);
    TextureAssetSubResource_s Res;
    Res.DataOffset = 0;
    Res.RowPitch = static_cast<uint32_t>(RowBytes);
    Res.SlicePitch = static_cast<uint32_t>(NumBytes);

    Asset->SubResourceInfos.emplace_back(Res);

    Asset->Data.resize(NumBytes);
    memcpy(Asset->Data.data(), TexData, NumBytes);

    STBI_FREE(TexData);

    return true;
}

bool LoadPNGTexture(const char* FilePath, TextureAsset_s* Asset)
{
    return LoadSTBITexture(FilePath, Asset);
}

bool LoadBMPTexture(const char* FilePath, TextureAsset_s* Asset)
{
    return LoadSTBITexture(FilePath, Asset);
}

// TODO: Currently converts to LDR on return, we will want to retain hdr
bool LoadHDRTexture(const char* FilePath, TextureAsset_s* Asset)
{
    return LoadSTBITexture(FilePath, Asset);
}

bool LoadTGATexture(const char* FilePath, TextureAsset_s* Asset)
{
    return LoadSTBITexture(FilePath, Asset);
}

bool LoadJPEGTexture(const char* FilePath, TextureAsset_s* Asset)
{
    return LoadSTBITexture(FilePath, Asset);
}

TextureAsset_s* LoadTextureInternal(const char* FilePath, const wchar_t* WidePath, bool GenerateMips)
{
    std::wstring CookedPath = ReplacePathExtension(WidePath, L"rtex");

    if (TextureAsset_s* Found = GTextureAssets.FindAsset(CookedPath))
    {
        return Found;
    }

    TextureAsset_s* Asset = GTextureAssets.CreateAsset(CookedPath);

    std::wstring Extension = GetPathExtension(WidePath);

    bool Success = LoadRTexTexture(WidePath, Asset);
    
    if(!Success)
    {
        if (Extension == L"png")
        {
            Success = LoadPNGTexture(FilePath, Asset);
        }
        else if (Extension == L"bmp")
        {
            Success = LoadBMPTexture(FilePath, Asset);
        }
        else if (Extension == L"hdr")
        {
            Success = LoadHDRTexture(FilePath, Asset);
        }
        else if (Extension == L"tga")
        {
            Success = LoadTGATexture(FilePath, Asset);
        }
        else if (Extension == L"jpeg" || Extension == L"jpg")
        {
            Success = LoadJPEGTexture(FilePath, Asset);
        }
        //else if (Extension == L"dds")
        //{
        //    Success = LoadDDSTexture(WidePath, Asset);
        //}
        else
        {
            LOGERROR("Unsupported texture extension: %S ", WidePath);
            return nullptr;
        }
    }

    Asset->SourcePath = WidePath;

    if (Extension != L"rtex")
    {
        StreamRTex(CookedPath.c_str(), FileStreamMode_e::WRITE, Asset);
    }

    return Asset;
}

TextureAsset_s* LoadTextureAsset(const std::wstring& FilePath)
{
    TextureAsset_s* Asset = nullptr;

    std::string NarrowPath = WideToNarrow(FilePath);
    Asset = LoadTextureInternal(NarrowPath.c_str(), FilePath.c_str(), false);

    return Asset;
}
