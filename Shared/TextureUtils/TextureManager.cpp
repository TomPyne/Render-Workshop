#include "TextureManager.h"

#include "FileUtils/FileStream.h"
#include "FileUtils/PathUtils.h"
#include "Logging/Logging.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <Render/Binding.h>
#include <Render/Textures.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

// TODO:
// 1. Enqueue texture loads on a helper thread, return the Texture_t handle immediately.
// 2. Default textures

struct TextureManagerGlobals
{
    std::map<std::wstring, std::unique_ptr<TextureAsset_s>> LoadedTextureAssets;
} G;

// Move this to another util folder if required
std::string WideToNarrow(const std::wstring& WideStr)
{
    std::string NarrowStr;
    NarrowStr.resize(WideStr.length());

    std::transform(WideStr.begin(), WideStr.end(), NarrowStr.begin(), [](wchar_t WideChar)
    {
        return (char)WideChar;
    });

    return NarrowStr;
}

std::wstring NarrowToWide(const std::string& NarrowStr)
{
    std::wstring WideStr;
    WideStr.resize(NarrowStr.length());

    std::transform(NarrowStr.begin(), NarrowStr.end(), WideStr.begin(), [](char NarrowChar)
    {
        return (wchar_t)NarrowChar;
    });

    return WideStr;
}

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

TextureAsset_s* LoadTextureInternal(const char* FilePath, const wchar_t* WidePath, bool GenerateMips)
{
    auto Found = G.LoadedTextureAssets.find(WidePath);
    if (Found != G.LoadedTextureAssets.end())
    {
        return Found->second.get();
    }

    int X, Y, Comp;
    void* TexData = stbi_load(FilePath, &X, &Y, &Comp, 4);

    if (!TexData)
    {
        LOGERROR("Failed to load texture %s", FilePath);
        return {};
    }

    //ASSERTMSG(Comp == 4, "Loaded texture does not have the required component count of 4 (%i)", Comp);

    if (GenerateMips)
    {
        if (!IsPowerOfTwo(X) || !IsPowerOfTwo(Y))
        {
            LOGWARNING("Unsupported: Texture %s cannot generate mips as it is not power of 2 sized");
            GenerateMips = false;
        }
    }

    uint32_t MipX = static_cast<uint32_t>(X);
    uint32_t MipY = static_cast<uint32_t>(Y);

    TextureAsset_s* Asset = new TextureAsset_s;
    G.LoadedTextureAssets.emplace(std::wstring(WidePath), std::unique_ptr<TextureAsset_s>(Asset));

    const tpr::RenderFormat TextureFormat = tpr::RenderFormat::R8G8B8A8_UNORM;

    size_t RowPitch, SlicePitch;
    CalculateTexturePitch(TextureFormat, MipX, MipY, &RowPitch, &SlicePitch);

    Asset->MipPixels.push_back({});
    Asset->MipPixels[0].resize(SlicePitch);

    std::memcpy(Asset->MipPixels[0].data(), TexData, SlicePitch);

    if (GenerateMips)
    {
        const uint8_t* LastMipData = Asset->MipPixels[0].data();

        while (MipX >= 2 && MipY >= 2)
        {
            MipX /= 2u;
            MipY /= 2u;

            CalculateTexturePitch(TextureFormat, MipX, MipY, &RowPitch, &SlicePitch);

            Asset->MipPixels.push_back({});
            Asset->MipPixels.back().resize(SlicePitch);
            uint8_t* CurrentMipData = Asset->MipPixels.back().data();

            for (uint32_t YIt = 0u; YIt < MipY; YIt++)
            {
                for (uint32_t XIt = 0u; XIt < MipX * 4; XIt++)
                {
                    float Sample00 = static_cast<float>(LastMipData[(YIt * 2) * (MipX * 2 * 4) + (XIt * 2)]) / 255.0f;
                    float Sample01 = static_cast<float>(LastMipData[(YIt * 2) * (MipX * 2 * 4) + (XIt * 2 + 1)]) / 255.0f;
                    float Sample10 = static_cast<float>(LastMipData[(YIt * 2 + 1) * (MipX * 2 * 4) + (XIt * 2)]) / 255.0f;
                    float Sample11 = static_cast<float>(LastMipData[(YIt * 2 + 1) * (MipX * 2 * 4) + (XIt * 2 + 1)]) / 255.0f;
                    CurrentMipData[YIt * MipY * 4 + XIt] = static_cast<uint8_t>(((Sample00 + Sample01 + Sample10 + Sample11) / 4.0f) * 255.0f);
                }
            }

            LastMipData = CurrentMipData;
        }
    }

    // Write generated asset to disk
    {
        Asset->SourcePath = ReplacePathExtension(WidePath, L"rtex");

        OFileStream_s Stream(Asset->SourcePath);

        if (Stream.IsOpen())
        {
            Stream.Write(&Asset->Width);
            Stream.Write(&Asset->Height);
            Stream.Write(&Asset->Format);
            Stream.Write(&Asset->MipCount);

            for (const auto& AssetMipData : Asset->MipPixels)
            {
                Stream.WriteArray(AssetMipData.data(), AssetMipData.size());
            }
        }
        else
        {
            LOGERROR("Failed to write texture asset file: %S", Asset->SourcePath);
        }
    }

    return Asset;
}

TextureAsset_s* LoadTextureAsset(const std::wstring& FilePath)
{
    TextureAsset_s* Asset = nullptr;

    if (HasPathExtension(FilePath, L".rtex")) // Load from asset
    {
        IFileStream_s Stream(FilePath);
        if (!Stream.IsOpen())
        {
            LOGERROR("Invalid path, has the texture been generated?");
            return nullptr;
        }

        Asset = new TextureAsset_s;
        G.LoadedTextureAssets.emplace(FilePath, std::unique_ptr<TextureAsset_s>(Asset));

        Stream.Read(&Asset->Width);
        Stream.Read(&Asset->Height);
        Stream.Read(&Asset->Format);
        Stream.Read(&Asset->MipCount);

        Asset->MipPixels.resize(Asset->MipCount);

        uint32_t MipWidth = Asset->Width;
        uint32_t MipHeight = Asset->Height;
        for (uint32_t MipIt = 0; MipIt < Asset->MipCount; MipIt++)
        {
            CHECK(MipWidth >= 1 && MipHeight >= 1);

            size_t RowPitch, SlicePitch;
            CalculateTexturePitch(Asset->Format, MipWidth, MipHeight, &RowPitch, &SlicePitch);

            Asset->MipPixels[MipIt].resize(SlicePitch);

            Stream.ReadArray(Asset->MipPixels[MipIt].data(), SlicePitch);

            MipWidth /= 2;
            MipHeight /= 2;
        }

        Asset->SourcePath = FilePath;
    }
    else // a source asset path, generate an asset
    {
        std::string NarrowPath = WideToNarrow(FilePath);
        Asset = LoadTextureInternal(NarrowPath.c_str(), FilePath.c_str(), false);
    }

    return Asset;
}
