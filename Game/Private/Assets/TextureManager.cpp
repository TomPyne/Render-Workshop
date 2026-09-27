#include "Assets/TextureManager.h"

#include "Assets/AssetManager.h"
#include "Rendering/Texture.h"

#include <Render/Render.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Jobs/JobSystem.h>
#include <Shared/Logging/Logging.h>
#include <Shared/TextureUtils/DDSTextureLoader.h>

#include <unordered_map>
#include <vector>

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
    static const Path_s ErrorMeshPath = Path_s(PathDirectory_e::Assets, L"Game", L"Textures/Error.hp_tex");
    return RequestTexture(ErrorMeshPath, false, true);
}

enum class TextureSourceType_e
{
    STB,
    DDS,
};

struct TextureLoadRequest_s
{
    Path_s SourcePath;
    TextureSourceType_e SourceType = TextureSourceType_e::STB;
    rl::TextureDimension Dimension = rl::TextureDimension::TEX2D;

    // STB only, DDS takes its format from the file
    TextureFormat_e Format = TextureFormat_e::Unknown;
    bool HasAlpha = false;
};

bool ParseTextureDimension(const std::string& Name, rl::TextureDimension& OutDimension)
{
    if (Name == "Tex2D")
    {
        OutDimension = rl::TextureDimension::TEX2D;
        return true;
    }

    if (Name == "Tex2DArray")
    {
        OutDimension = rl::TextureDimension::TEX2D_ARRAY;
        return true;
    }

    if (Name == "Tex3D")
    {
        OutDimension = rl::TextureDimension::TEX3D;
        return true;
    }

    return false;
}

rl::TextureDimension DDSToTextureDimension(const DDSTexture_s& Dds)
{
    switch (Dds.Dimension)
    {
    case DDSTexture_s::Dimension_e::ONEDIM:
        return Dds.DepthOrArraySize > 1 ? rl::TextureDimension::TEX1D_ARRAY : rl::TextureDimension::TEX1D;
    case DDSTexture_s::Dimension_e::TWODIM:
        return Dds.DepthOrArraySize > 1 ? rl::TextureDimension::TEX2D_ARRAY : rl::TextureDimension::TEX2D;
    case DDSTexture_s::Dimension_e::THREEDIM:
        return rl::TextureDimension::TEX3D;
    }

    return rl::TextureDimension::UNKNOWN;
}

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

// Main thread. Validates the asset json without touching the source file
bool ParseTextureRequestDDS(const JsonValue_s& Data, TextureLoadRequest_s& OutRequest)
{
    // Required, and parsed first, so the 2D fallback is skipped for other dimensions even if a later field fails
    std::string DimensionName;
    if (!ENSUREMSG(JsonHelpers::ParseString(Data, "Dimension", DimensionName), "[TextureManager::ParseTextureRequestDDS] Missing Dimension field"))
    {
        return false;
    }

    if (!ParseTextureDimension(DimensionName, OutRequest.Dimension))
    {
        LOGWARNING("[TextureManager::ParseTextureRequestDDS] Texture contains an invalid dimension '%s'", DimensionName.c_str());
        return false;
    }

    if (!ENSUREMSG(JsonHelpers::ParsePath(Data, "SourceFilePath", OutRequest.SourcePath), "[TextureManager::ParseTextureRequestDDS] Missing SourceFile field"))
    {
        return false;
    }

    return true;
}

// Any thread. Only writes OutTexture on success
bool BuildTextureDDS(const TextureLoadRequest_s& Request, Texture_s& OutTexture)
{
    const Path_s& Path = Request.SourcePath;

    LOGINFO("[TextureManager::BuildTextureDDS] Building texture: %s", Path.ToString().c_str());

    DDSTexture_s Dds;
    if (!LoadDDSTexture(Path.ToWString().c_str(), &Dds))
    {
        LOGWARNING("[TextureManager::BuildTextureDDS] Failed to load texture : %s", Path.ToString().c_str());
        return false;
    }

    if (Dds.Cubemap)
    {
        LOGWARNING("[TextureManager::BuildTextureDDS] Cubemaps are not supported : %s", Path.ToString().c_str());
        return false;
    }

    // A single mip volume is stored as contiguous depth slices, the same layout as a 2D array
    const bool VolumeAsArray = Request.Dimension == rl::TextureDimension::TEX2D_ARRAY && Dds.Dimension == DDSTexture_s::Dimension_e::THREEDIM;

    if (VolumeAsArray && Dds.MipCount != 1)
    {
        LOGWARNING("[TextureManager::BuildTextureDDS] Volume textures can only be loaded as an array with a single mip : %s", Path.ToString().c_str());
        return false;
    }

    if (!VolumeAsArray && DDSToTextureDimension(Dds) != Request.Dimension)
    {
        LOGWARNING("[TextureManager::BuildTextureDDS] Texture dimension does not match the asset : %s", Path.ToString().c_str());
        return false;
    }

    std::vector<rl::MipData> Subresources;

    if (VolumeAsArray)
    {
        const DDSTexture_s::Subresource_s& Src = Dds.SubResourceInfos[0];
        Subresources.reserve(Dds.DepthOrArraySize);

        for (u32 SliceIt = 0; SliceIt < Dds.DepthOrArraySize; SliceIt++)
        {
            rl::MipData& Mip = Subresources.emplace_back();
            Mip.Data = Dds.Data.data() + Src.DataOffset + static_cast<size_t>(Src.SlicePitch) * SliceIt;
            Mip.RowPitch = Src.RowPitch;
            Mip.SlicePitch = Src.SlicePitch;
        }
    }
    else
    {
        Subresources.reserve(Dds.SubResourceInfos.size());

        for (const DDSTexture_s::Subresource_s& Src : Dds.SubResourceInfos)
        {
            rl::MipData& Mip = Subresources.emplace_back();
            Mip.Data = Dds.Data.data() + Src.DataOffset;
            Mip.RowPitch = Src.RowPitch;
            Mip.SlicePitch = Src.SlicePitch;
        }
    }

    rl::TextureCreateDescEx Desc = {};
    Desc.Width = Dds.Width;
    Desc.Height = Dds.Height;
    Desc.DepthOrArraySize = Dds.DepthOrArraySize;
    Desc.MipCount = Dds.MipCount;
    Desc.Dimension = Request.Dimension;
    Desc.Flags = rl::RenderResourceFlags::SRV;
    Desc.ResourceFormat = Dds.Format;
    Desc.Data = Subresources.data();
    Desc.DebugName = Path.ToWString();

    // The source data is copied into an upload buffer before this returns
    rl::TexturePtr Texture = rl::CreateTextureEx(Desc);

    if (!Texture)
    {
        LOGWARNING("[TextureManager::BuildTextureDDS] Texture failed to upload to GPU: %s", Path.ToString().c_str());
        return false;
    }

    rl::ShaderResourceViewPtr SRV = rl::CreateTextureSRV(Texture);

    if (!SRV)
    {
        LOGWARNING("[TextureManager::BuildTextureDDS] Texture failed to create SRV: %s", Path.ToString().c_str());
        return false;
    }

    OutTexture.Texture = std::move(Texture);
    OutTexture.SRV = std::move(SRV);
    OutTexture.Size = uint2(Dds.Width, Dds.Height);
    OutTexture.DepthOrArraySize = Dds.DepthOrArraySize;
    OutTexture.Format = Dds.Format;
    OutTexture.MipCount = Dds.MipCount;
    OutTexture.Dimension = Request.Dimension;

    return true;
}

bool BuildTexture(const TextureLoadRequest_s& Request, Texture_s& OutTexture)
{
    switch (Request.SourceType)
    {
    case TextureSourceType_e::STB:
        return BuildTextureSTB(Request, OutTexture);
    case TextureSourceType_e::DDS:
        return BuildTextureDDS(Request, OutTexture);
    }

    return false;
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

    TextureLoadRequest_s Request;
    bool RequestParsed = false;

    if (FileFormat == "png" || FileFormat == "tga")
    {
        Request.SourceType = TextureSourceType_e::STB;
        RequestParsed = ParseTextureRequestSTB(Data, Request);
    }
    else if (FileFormat == "dds")
    {
        Request.SourceType = TextureSourceType_e::DDS;
        RequestParsed = ParseTextureRequestDDS(Data, Request);
    }
    else
    {
        LOGWARNING("[TextureManager::RequestTexture] Unsupported file format for texture: %s", FileFormat.c_str());
    }

    // The error texture is 2D, binding it in place of another dimension would mismatch the shader's view
    const bool UseFallback = ErrorTextureIfMissing && Request.Dimension == rl::TextureDimension::TEX2D;

    if (!RequestParsed)
    {
        return UseFallback ? RequestErrorTexture() : nullptr;
    }

    // Cached before the load so repeat requests share this texture rather than loading it again
    std::shared_ptr<Texture_s> NewTexture = std::make_shared<Texture_s>();
    AssetManager_c::CacheTexture(Data.GetHash(), NewTexture);

    // Fetched here as the job must not touch the asset manager
    std::shared_ptr<Texture_s> Fallback = UseFallback ? RequestErrorTexture() : nullptr;

    auto LoadJob = [NewTexture, Request = std::move(Request), Fallback = std::move(Fallback)]()
    {
        if (!BuildTexture(Request, *NewTexture))
        {
            if (!Fallback)
                return;

            NewTexture->Texture = Fallback->Texture;
            NewTexture->SRV = Fallback->SRV;
            NewTexture->Size = Fallback->Size;
            NewTexture->DepthOrArraySize = Fallback->DepthOrArraySize;
            NewTexture->Format = Fallback->Format;
            NewTexture->MipCount = Fallback->MipCount;
            NewTexture->Dimension = Fallback->Dimension;
        }

        NewTexture->Ready.store(true, std::memory_order_release);
    };

    if (Blocking)
    {
        JobWaitOrHelp(JobSubmit(std::move(LoadJob), "LoadTexture"));
    }
    else
    {
        JobSubmitDetached(std::move(LoadJob), "LoadTexture");
    }

    return NewTexture;
}

}