#include "Assets/TextureManager.h"

#include "Assets/AssetManager.h"
#include "Rendering/Texture.h"

#include <Render/Render.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Jobs/JobSystem.h>
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
    return RequestTexture(ErrorMeshPath, false, true);
}

struct TextureLoadRequest_s
{
    Path_s SourcePath;
    TextureFormat_e Format = TextureFormat_e::Unknown;
    bool HasAlpha = false;
};

// Main thread. Validates the asset json without touching the source file
bool ParseTextureRequestSTB(const JsonValue_s& Data, TextureLoadRequest_s& OutRequest)
{
    if (!ENSUREMSG(JsonHelpers::ParsePath(Data, "SourceFilePath", OutRequest.SourcePath), "[TextureManager::ParseTextureRequestSTB] Missing SourceFile field"))
    {
        return false;
    }

    int TexFormatRaw = 0;
    if (!ENSUREMSG(JsonHelpers::ParseInt(Data, "Format", TexFormatRaw), "[TextureManager::ParseTextureRequestSTB] Missing Format field"))
    {
        return false;
    }

    OutRequest.Format = static_cast<TextureFormat_e>(TexFormatRaw);
    if (OutRequest.Format >= TextureFormat_e::Count || OutRequest.Format == TextureFormat_e::Unknown)
    {
        LOGWARNING("[TextureManager::ParseTextureRequestSTB] Texture contains an invalid format : %s", OutRequest.SourcePath.ToString().c_str());
        return false;
    }

    JsonHelpers::ParseBool(Data, "HasAlpha", OutRequest.HasAlpha);

    return true;
}

// Any thread. Only writes OutTexture on success
bool BuildTextureSTB(const TextureLoadRequest_s& Request, Texture_s& OutTexture)
{
    const Path_s& Path = Request.SourcePath;

    LOGINFO("[TextureManager::BuildTextureSTB] Building texture: %s", Path.ToString().c_str());

    int X, Y, Channels;
    stbi_uc* RawData = stbi_load(Path.ToString().c_str(), &X, &Y, &Channels, 4);

    if (!RawData)
    {
        LOGWARNING("[TextureManager::BuildTextureSTB] Failed to load texture : %s", Path.ToString().c_str());
        return false;
    }

    if (X <= 0 || Y <= 0 || Channels <= 0)
    {
        LOGWARNING("[TextureManager::BuildTextureSTB] Texture has 0 dimension : %s", Path.ToString().c_str());
        stbi_image_free(RawData);
        return false;
    }

    if (Channels < TexFormatRequiredChannels(Request.Format, Request.HasAlpha)) // TODO: Texture optional alpha
    {
        LOGWARNING("[TextureManager::BuildTextureSTB] Loaded Texture does not contain the required number of channels: %s", Path.ToString().c_str());
        stbi_image_free(RawData);
        return false;
    }

    const uint2 Size = uint2(static_cast<u32>(X), static_cast<u32>(Y));
    const rl::RenderFormat Format = TexToRenderFormat(Request.Format);

    rl::TextureCreateDesc Desc = {};
    rl::MipData Mip0(RawData, Format, Size.x, Size.y);

    Desc.Data = &Mip0;
    Desc.DebugName = Path.ToWString();
    Desc.Flags = rl::RenderResourceFlags::SRV;
    Desc.Format = Format;
    Desc.Width = Size.x;
    Desc.Height = Size.y;

    rl::TexturePtr Texture = rl::CreateTexture(Desc);

    stbi_image_free(RawData);
    if (!Texture)
    {
        LOGWARNING("[TextureManager::BuildTextureSTB] Texture failed to upload to GPU: %s", Path.ToString().c_str());
        return false;
    }

    rl::ShaderResourceViewPtr SRV = rl::CreateTextureSRV(Texture);

    if (!SRV)
    {
        LOGWARNING("[TextureManager::BuildTextureSTB] Texture failed to create SRV: %s", Path.ToString().c_str());
        return false;
    }

    OutTexture.Texture = std::move(Texture);
    OutTexture.SRV = std::move(SRV);
    OutTexture.Size = Size;
    OutTexture.Format = Format;
    OutTexture.MipCount = 1; // TODO: Texture mips

    return true;
}

std::shared_ptr<Texture_s> RequestTexture(const Path_s& Path, bool ErrorTextureIfMissing, bool Blocking)
{
    Json_t Json;
    const bool JsonLoaded = LoadJsonFromFile(Path.ToWString(), Json);
    if (!JsonLoaded)
    {
        LOGWARNING("[TextureManager::RequestTexture] Failed to load Texture json from path %S", Path.ToWString().c_str());
        return ErrorTextureIfMissing ? RequestErrorTexture() : nullptr;
    }

    return RequestTexture(Json, ErrorTextureIfMissing, Blocking);
}

std::shared_ptr<Texture_s> RequestTexture(const JsonValue_s& Data, bool ErrorTextureIfMissing, bool Blocking)
{
    std::shared_ptr<Texture_s> Texture = AssetManager_c::TryGetTexture(Data.GetHash());
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
        TextureLoadRequest_s Request;
        if (!ParseTextureRequestSTB(Data, Request))
        {
            return ErrorTextureIfMissing ? RequestErrorTexture() : nullptr;
        }

        // Cached before the load so repeat requests share this texture rather than loading it again
        std::shared_ptr<Texture_s> NewTexture = std::make_shared<Texture_s>();
        AssetManager_c::CacheTexture(Data.GetHash(), NewTexture);

        // Fetched here as the job must not touch the asset manager
        std::shared_ptr<Texture_s> Fallback = ErrorTextureIfMissing ? RequestErrorTexture() : nullptr;

        auto LoadJob = [NewTexture, Request = std::move(Request), Fallback = std::move(Fallback)]()
        {
            if (!BuildTextureSTB(Request, *NewTexture))
            {
                if (!Fallback)
                    return;

                NewTexture->Texture = Fallback->Texture;
                NewTexture->SRV = Fallback->SRV;
                NewTexture->Size = Fallback->Size;
                NewTexture->Format = Fallback->Format;
                NewTexture->MipCount = Fallback->MipCount;
            }

            NewTexture->Ready.store(true, std::memory_order_release);
        };

        if (Blocking)
        {
            JobWaitOrHelp(JobSubmit(std::move(LoadJob), "LoadTextureSTB"));
        }
        else
        {
            JobSubmitDetached(std::move(LoadJob), "LoadTextureSTB");
        }

        return NewTexture;
    }

    LOGWARNING("[TextureManager::RequestTexture] Unsupported file format for texture: %s", FileFormat.c_str());
    return ErrorTextureIfMissing ? RequestErrorTexture() : nullptr;
}

}