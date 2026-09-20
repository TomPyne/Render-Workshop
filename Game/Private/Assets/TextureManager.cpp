#include "Assets/TextureManager.h"

#include "Rendering/Texture.h"

#include <Render/Render.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

#include <unordered_map>

#include <stb/stb_image.h>

#define TEXTURE_ASSET_VERSION_INITIAL 1
#define TEXTURE_ASSET_VERSION_CURRENT TEXTURE_ASSET_VERSION_INITIAL

namespace TextureManager
{

enum class TextureFormat_e
{
    Unknown = 0,
    RGBA8, // 1
    Count
};

struct TextureManagerGlobals_s
{
    // Content addressable storage of loaded meshes.
    std::unordered_map<uint64_t, std::shared_ptr<Texture_s>> LoadedTextures;

    std::shared_ptr<Texture_s> TryGet(const JsonValue_s& Data)
    {
        // Compute hash of data to use as a key for caching
        auto It = LoadedTextures.find(Data.GetHash());
        if (It != LoadedTextures.end())
        {
            return It->second;
        }

        return nullptr;
    }
} G;

constexpr rl::RenderFormat TexToRenderFormat(TextureFormat_e TexFormat)
{
    switch (TexFormat)
    {
    case TextureFormat_e::RGBA8:
        return rl::RenderFormat::R8G8B8A8_UNORM;
    }

    ENSUREMSG(false, "[TextureManager::TexFormatRequiredChannels] Unsupported type");
    return rl::RenderFormat::UNKNOWN;
}

constexpr int TexFormatRequiredChannels(TextureFormat_e TexFormat, bool HasAlpha)
{
    switch (TexFormat)
    {
    case TextureFormat_e::RGBA8:
        return HasAlpha ? 4 : 3;
    }

    ENSUREMSG(false, "[TextureManager::TexFormatRequiredChannels] Unsupported type");
    return 0;
}

std::shared_ptr<Texture_s> RequestErrorTexture()
{
    static const Path_s ErrorMeshPath = Path_s(PathDirectory_e::Assets, L"Game", L"Textures/ErrorTexture.hp_tex");
    return RequestTexture(ErrorMeshPath, false);
}

std::shared_ptr<Texture_s> RequestTextureSTB(const JsonValue_s& Data)
{
    Path_s Path;
    if (!ENSUREMSG(JsonHelpers::ParsePath(Data, "SourceFilePath", Path), "[TextureManager::RequestTextureSTB] Missing SourceFile field"))
    {
        return nullptr;
    }

    int TexFormatRaw = 0;
    if (!ENSUREMSG(JsonHelpers::ParseInt(Data, "Format", TexFormatRaw), "[TextureManager::RequestTextureSTB] Missing Format field"))
    {
        return nullptr;
    }

    const TextureFormat_e TexFormat = static_cast<TextureFormat_e>(TexFormatRaw);
    if (TexFormat >= TextureFormat_e::Count || TexFormat == TextureFormat_e::Unknown)
    {
        LOGWARNING("[TextureManager::RequestTextureSTB] Texture contains an invalid format : %s", Path.ToString().c_str());
        return nullptr;
    }

    LOGINFO("[TextureManager::RequestTextureSTB] Building texture: %s", Path.ToString().c_str());

    int X, Y, Channels;
    stbi_uc* RawData = stbi_load(Path.ToString().c_str(), &X, &Y, &Channels, 4);

    if (!RawData)
    {
        LOGWARNING("[TextureManager::RequestTextureSTB] Failed to load texture : %s", Path.ToString().c_str());
        return nullptr;
    }

    if (X <= 0 || Y <= 0 || Channels <= 0)
    {
        LOGWARNING("[TextureManager::RequestTextureSTB] Texture has 0 dimension : %s", Path.ToString().c_str());
        stbi_image_free(RawData);
        return nullptr;
    }

    bool HasAlpha = false;
    JsonHelpers::ParseBool(Data, "HasAlpha", HasAlpha);

    if (Channels < TexFormatRequiredChannels(TexFormat, HasAlpha)) // TODO: Texture optional alpha
    {
        LOGWARNING("[TextureManager::RequestTextureSTB] Loaded Texture does not contain the required number of channels: %s", Path.ToString().c_str());
        stbi_image_free(RawData);
        return nullptr;
    }

    std::shared_ptr<Texture_s> NewTexture = std::make_shared<Texture_s>();

    NewTexture->Size.x = static_cast<u32>(X);
    NewTexture->Size.y = static_cast<u32>(Y);
    NewTexture->Format = TexToRenderFormat(TexFormat);
    NewTexture->MipCount = 1; // TODO: Texture mips

    rl::TextureCreateDesc Desc = {};
    rl::MipData Mip0(RawData, NewTexture->Format, NewTexture->Size.x, NewTexture->Size.y);

    Desc.Data = &Mip0;
    Desc.DebugName = Path.ToWString();
    Desc.Flags = rl::RenderResourceFlags::SRV;
    Desc.Format = NewTexture->Format;
    Desc.Width = NewTexture->Size.x;
    Desc.Height = NewTexture->Size.y;

    NewTexture->Texture = rl::CreateTexture(Desc);
    if (!NewTexture->Texture)
    {
        LOGWARNING("[TextureManager::RequestTextureSTB] Texture failed to upload to GPU: %s", Path.ToString().c_str());
        return nullptr;
    }

    NewTexture->SRV = rl::CreateTextureSRV(NewTexture->Texture);

    if (!NewTexture->SRV)
    {
        LOGWARNING("[TextureManager::RequestTextureSTB] Texture failed to create SRV: %s", Path.ToString().c_str());
        return nullptr;
    }

    return NewTexture;
}

std::shared_ptr<Texture_s> RequestTexture(const Path_s& Path, bool ErrorTextureIfMissing)
{
    Json_t Json;
    const bool JsonLoaded = LoadJsonFromFile(Path.ToWString(), Json);
    if (!JsonLoaded)
    {
        LOGWARNING("[TextureManager::RequestTexture] Failed to load Texture json from path %S", Path.ToWString().c_str());
        return ErrorTextureIfMissing ? RequestErrorTexture() : nullptr;
    }

    return RequestTexture(Json, ErrorTextureIfMissing);
}

std::shared_ptr<Texture_s> RequestTexture(const JsonValue_s& Data, bool ErrorTextureIfMissing)
{
    std::shared_ptr<Texture_s> Texture = G.TryGet(Data);
    if (Texture)
        return Texture;

    int32_t Version = -1;
    JsonHelpers::ParseInt(Data, "Version", Version);
    if (Version != TEXTURE_ASSET_VERSION_CURRENT)
    {
        LOGWARNING("[TextureManager::RequestTexture] Unsupported texture asset version: %d", Version);
        return ErrorTextureIfMissing ? RequestErrorTexture() : nullptr;
    }

    std::string FileFormat;
    if (!JsonHelpers::ParseString(Data, "SourceFileType", FileFormat))
    {
        LOGWARNING("[TextureManager::RequestTexture] Missing FileFormat field");
        return ErrorTextureIfMissing ? RequestErrorTexture() : nullptr;
    }

    if (FileFormat == "png" || FileFormat == "tga")
    {
        std::shared_ptr<Texture_s> NewTexture = RequestTextureSTB(Data);
        if (!NewTexture)
        {
            return ErrorTextureIfMissing ? RequestErrorTexture() : nullptr;
        }
        else
        {
            return NewTexture;
        }
    }

    LOGWARNING("[TextureManager::RequestTexture] Unsupported file format for texture: %s", FileFormat.c_str());
    return ErrorTextureIfMissing ? RequestErrorTexture() : nullptr;
}

}