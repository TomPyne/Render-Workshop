#include "TextureManager.h"

#include "Logging/Logging.h"

#include <algorithm>
#include <map>
#include <Render/Binding.h>
#include <Render/Textures.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

// TODO:
// 1. Texture manager shouldn't hold an owning reference to textures if possible, it prevents them from ever unloading.
//    This may need to be implented with a new type of weak pointer in the Render lib.
// 2. Enqueue texture loads on a helper thread, return the Texture_t handle immediately.
// 3. Default textures

struct TextureManagerGlobals
{
    std::map<std::string, TextureAsset_s> LoadedTextures;
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

TextureAsset_s LoadTextureInternal(const char* FilePath, const wchar_t* WidePath, bool GenerateMips)
{
    auto Found = G.LoadedTextures.find(FilePath);
    if (Found != G.LoadedTextures.end())
    {
        return Found->second;
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
    std::vector<tpr::MipData> MipData;
    std::vector<std::unique_ptr<uint8_t[]>> MipMemory;

    MipData.emplace_back(TexData, tpr::RenderFormat::R8G8B8A8_UNORM, MipX, MipY);

    if (GenerateMips)
    {
        const uint8_t* LastMipData = (uint8_t*)TexData;

        while (MipX >= 2 && MipY >= 2)
        {
            MipX /= 2u;
            MipY /= 2u;

            MipMemory.push_back(std::make_unique<uint8_t[]>(MipX * MipY * 4u));
            uint8_t* CurrentMipData = MipMemory.back().get();

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

            MipData.emplace_back(CurrentMipData, tpr::RenderFormat::R8G8B8A8_UNORM, MipX, MipY);

            LastMipData = CurrentMipData;
        }
    }

    tpr::TextureCreateDescEx TexDesc = {};
    TexDesc.Data = MipData.data();
    TexDesc.DepthOrArraySize = 1u;
    TexDesc.Dimension = tpr::TextureDimension::TEX2D;
    TexDesc.Flags = tpr::RenderResourceFlags::SRV;
    TexDesc.Height = Y;
    TexDesc.Width = X;
    TexDesc.MipCount = static_cast<uint32_t>(MipData.size());
    TexDesc.DebugName = WidePath;
    TexDesc.ResourceFormat = tpr::RenderFormat::R8G8B8A8_UNORM;

    TextureAsset_s Asset;
    Asset.Texture = tpr::CreateTextureEx(TexDesc);

    STBI_FREE(TexData);

    if (!Asset.Texture)
    {
        return {};
    }

    Asset.SRV = tpr::CreateTextureSRV(Asset.Texture, tpr::RenderFormat::R8G8B8A8_UNORM, tpr::TextureDimension::TEX2D, TexDesc.MipCount, TexDesc.DepthOrArraySize);

    if (!Asset.SRV)
    {
        return {};
    }

#if _DEBUG
    Asset.DebugName = WidePath;
#endif

    G.LoadedTextures[FilePath] = Asset;

    return Asset;
}

TextureAsset_s LoadTexture(const std::wstring& FilePath, bool GenerateMips)
{
    std::string NarrowPath = WideToNarrow(FilePath);
    return LoadTextureInternal(NarrowPath.c_str(), FilePath.c_str(), GenerateMips);
}

TextureAsset_s LoadTexture(const std::string& FilePath, bool GenerateMips)
{
    std::wstring WidePath = NarrowToWide(FilePath);
    return LoadTextureInternal(FilePath.c_str(), WidePath.c_str(), GenerateMips);
}
