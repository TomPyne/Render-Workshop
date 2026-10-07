#include "Assets/TextureManager.h"

#include "Assets/AssetManager.h"
#include "Rendering/Texture.h"

#include <Render/Render.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Jobs/JobSystem.h>
#include <Shared/Logging/Logging.h>
#include <Shared/TextureUtils/DDSTextureLoader.h>

#include <cmath>
#include <cstring>
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

enum class MipFilter_e
{
    Color,  // RGB is sRGB encoded and averaged in linear space
    Linear,
    Normal, // RGB is a [0,1] encoded vector, renormalized after averaging
};

struct TextureLoadRequest_s
{
    Path_s SourcePath;
    TextureSourceType_e SourceType = TextureSourceType_e::STB;
    rl::TextureDimension Dimension = rl::TextureDimension::TEX2D;

    // STB only, DDS takes its format from the file
    TextureFormat_e Format = TextureFormat_e::Unknown;
    bool HasAlpha = false;
    bool GenMips = false;
    MipFilter_e MipFilter = MipFilter_e::Color;
};

bool ParseMipFilter(const std::string& Name, MipFilter_e& OutFilter)
{
    if (Name == "Color")
    {
        OutFilter = MipFilter_e::Color;
        return true;
    }

    if (Name == "Linear")
    {
        OutFilter = MipFilter_e::Linear;
        return true;
    }

    if (Name == "Normal")
    {
        OutFilter = MipFilter_e::Normal;
        return true;
    }

    return false;
}

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
    JsonHelpers::ParseBool(Data, "GenMips", OutRequest.GenMips);

    if (OutRequest.GenMips)
    {
        std::string MipFilterName;
        if (!ENSUREMSG(JsonHelpers::ParseString(Data, "MipFilter", MipFilterName), "[TextureManager::ParseTextureRequestSTB] Missing MipFilter field"))
        {
            return false;
        }

        if (!ParseMipFilter(MipFilterName, OutRequest.MipFilter))
        {
            LOGWARNING("[TextureManager::ParseTextureRequestSTB] Texture contains an invalid mip filter '%s' : %s", MipFilterName.c_str(), OutRequest.SourcePath.ToString().c_str());
            return false;
        }
    }

    return true;
}

float SRGBToLinear(float C)
{
    return C <= 0.04045f ? C / 12.92f : powf((C + 0.055f) / 1.055f, 2.4f);
}

float LinearToSRGB(float C)
{
    return C <= 0.0031308f ? C * 12.92f : 1.055f * powf(C, 1.0f / 2.4f) - 0.055f;
}

u8 UnormToByte(float V)
{
    return static_cast<u8>(Clamp(V, 0.0f, 1.0f) * 255.0f + 0.5f);
}

// Opposing normals can cancel to zero length, fall back to the tangent space up vector
float3 SafeNormalizeNormal(float3 V)
{
    const float Len = Length(V);
    return Len > 1e-5f ? V / Len : float3(0.0f, 0.0f, 1.0f);
}

float4 DecodeTexel(const u8* Texel, MipFilter_e Filter)
{
    const float4 Unorm = float4(Texel[0], Texel[1], Texel[2], Texel[3]) / 255.0f;

    switch (Filter)
    {
    case MipFilter_e::Color:
        return float4(SRGBToLinear(Unorm.x), SRGBToLinear(Unorm.y), SRGBToLinear(Unorm.z), Unorm.w);
    case MipFilter_e::Normal:
        return float4(SafeNormalizeNormal(Unorm.xyz * 2.0f - 1.0f), Unorm.w);
    case MipFilter_e::Linear:
        return Unorm;
    }

    return Unorm;
}

void EncodeTexel(float4 V, MipFilter_e Filter, u8* OutTexel)
{
    float3 RGB = V.xyz;

    switch (Filter)
    {
    case MipFilter_e::Color:
        RGB = float3(LinearToSRGB(RGB.x), LinearToSRGB(RGB.y), LinearToSRGB(RGB.z));
        break;
    case MipFilter_e::Normal:
        RGB = SafeNormalizeNormal(RGB) * 0.5f + 0.5f;
        break;
    case MipFilter_e::Linear:
        break;
    }

    OutTexel[0] = UnormToByte(RGB.x);
    OutTexel[1] = UnormToByte(RGB.y);
    OutTexel[2] = UnormToByte(RGB.z);
    OutTexel[3] = UnormToByte(V.w);
}

u32 CalculateMipCount(uint2 Size)
{
    u32 MipCount = 1;
    for (u32 Largest = Max(Size.x, Size.y); Largest > 1; Largest >>= 1)
    {
        MipCount++;
    }
    return MipCount;
}

uint2 CalculateMipSize(uint2 Size, u32 Mip)
{
    return uint2(Max(Size.x >> Mip, 1u), Max(Size.y >> Mip, 1u));
}

// Box filters SrcSize down to DstSize, keeping the unfiltered averages in OutAverages for the next level.
// An odd source dimension folds its last row or column into the final texel so nothing is dropped
template<typename FetchFunc>
void ReduceMip(uint2 SrcSize, uint2 DstSize, MipFilter_e Filter, FetchFunc&& Fetch, std::vector<float4>& OutAverages, u8* OutTexels)
{
    OutAverages.resize(static_cast<size_t>(DstSize.x) * DstSize.y);

    for (u32 Y = 0; Y < DstSize.y; Y++)
    {
        const u32 Y0 = Y * 2;
        const u32 Y1 = Y == DstSize.y - 1 ? SrcSize.y : Y0 + 2;

        for (u32 X = 0; X < DstSize.x; X++)
        {
            const u32 X0 = X * 2;
            const u32 X1 = X == DstSize.x - 1 ? SrcSize.x : X0 + 2;

            float4 Sum = float4(0.0f);
            for (u32 SrcY = Y0; SrcY < Y1; SrcY++)
            {
                for (u32 SrcX = X0; SrcX < X1; SrcX++)
                {
                    Sum += Fetch(static_cast<size_t>(SrcY) * SrcSize.x + SrcX);
                }
            }

            const size_t DstIndex = static_cast<size_t>(Y) * DstSize.x + X;
            const float4 Average = Sum / static_cast<float>((Y1 - Y0) * (X1 - X0));

            OutAverages[DstIndex] = Average;
            EncodeTexel(Average, Filter, OutTexels + DstIndex * 4);
        }
    }
}

// Returns every mip packed tightly as RGBA8. Each level averages the previous level's unnormalized values
// so normals match the average of the source texels beneath them
std::vector<u8> GenerateMipChain(const u8* Source, uint2 Size, u32 MipCount, MipFilter_e Filter)
{
    size_t TotalTexels = 0;
    for (u32 MipIt = 0; MipIt < MipCount; MipIt++)
    {
        const uint2 MipSize = CalculateMipSize(Size, MipIt);
        TotalTexels += static_cast<size_t>(MipSize.x) * MipSize.y;
    }

    std::vector<u8> Data(TotalTexels * 4);

    const size_t BaseTexels = static_cast<size_t>(Size.x) * Size.y;

    if (Filter == MipFilter_e::Normal)
    {
        for (size_t TexelIt = 0; TexelIt < BaseTexels; TexelIt++)
        {
            EncodeTexel(DecodeTexel(Source + TexelIt * 4, Filter), Filter, Data.data() + TexelIt * 4);
        }
    }
    else
    {
        memcpy(Data.data(), Source, BaseTexels * 4);
    }

    std::vector<float4> Prev;
    std::vector<float4> Next;
    u8* Dest = Data.data() + BaseTexels * 4;

    for (u32 MipIt = 1; MipIt < MipCount; MipIt++)
    {
        const uint2 SrcSize = CalculateMipSize(Size, MipIt - 1);
        const uint2 DstSize = CalculateMipSize(Size, MipIt);

        // The first level decodes straight from the source to avoid a full size float copy of it
        if (MipIt == 1)
        {
            ReduceMip(SrcSize, DstSize, Filter, [Source, Filter](size_t Index) { return DecodeTexel(Source + Index * 4, Filter); }, Next, Dest);
        }
        else
        {
            ReduceMip(SrcSize, DstSize, Filter, [&Prev](size_t Index) { return Prev[Index]; }, Next, Dest);
        }

        std::swap(Prev, Next);
        Dest += static_cast<size_t>(DstSize.x) * DstSize.y * 4;
    }

    return Data;
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

    const u32 MipCount = Request.GenMips ? CalculateMipCount(Size) : 1;

    std::vector<u8> MipChain;
    if (Request.GenMips)
    {
        MipChain = GenerateMipChain(RawData, Size, MipCount, Request.MipFilter);
    }

    const u8* TexelData = Request.GenMips ? MipChain.data() : RawData;

    std::vector<rl::MipData> Mips;
    Mips.reserve(MipCount);

    size_t MipOffset = 0;
    for (u32 MipIt = 0; MipIt < MipCount; MipIt++)
    {
        const uint2 MipSize = CalculateMipSize(Size, MipIt);
        const rl::MipData& Mip = Mips.emplace_back(TexelData + MipOffset, Format, MipSize.x, MipSize.y);
        MipOffset += Mip.SlicePitch;
    }

    rl::TextureCreateDescEx Desc = {};
    Desc.Width = Size.x;
    Desc.Height = Size.y;
    Desc.MipCount = MipCount;
    Desc.Dimension = rl::TextureDimension::TEX2D;
    Desc.Flags = rl::RenderResourceFlags::SRV;
    Desc.ResourceFormat = Format;
    Desc.Data = Mips.data();
    Desc.DebugName = Path.ToWString();

    // The source data is copied into an upload buffer before this returns
    rl::TexturePtr Texture = rl::CreateTextureEx(Desc);

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
    OutTexture.MipCount = MipCount;

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