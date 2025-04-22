#include "DDSTextureLoader.h"
#include "FileUtils/FileLoader.h"
#include "Logging/Logging.h"
#include <Render/TextureInfo.h>

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |       \
                ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */

#pragma pack(push,1)

constexpr uint32_t DDSMagic = 0x20534444; // "DDS "

#define DDS_FOURCC      0x00000004  // DDPF_FOURCC
#define DDS_RGB         0x00000040  // DDPF_RGB
#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDS_ALPHA       0x00000002  // DDPF_ALPHA
#define DDS_BUMPDUDV    0x00080000  // DDPF_BUMPDUDV

#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH

#define DDS_CUBEMAP 0x00000200 // DDSCAPS2_CUBEMAP

#define DDS_HEIGHT 0x00000002 // DDSD_HEIGHT

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
                                DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
                                DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )

struct DDSPixelFormat_s
{
    uint32_t    Size;
    uint32_t    Flags;
    uint32_t    FourCC;
    uint32_t    RGBBitCount;
    uint32_t    RBitMask;
    uint32_t    GBitMask;
    uint32_t    BBitMask;
    uint32_t    ABitMask;
};

struct DDSHeader_s
{
    uint32_t Size;
    uint32_t Flags;
    uint32_t Height;
    uint32_t Width;
    uint32_t PitchOrLinearSize;
    uint32_t Depth;
    uint32_t MipMapCount;
    uint32_t Reserved1[11];
    DDSPixelFormat_s DDSPixelFormat;
    uint32_t Caps;
    uint32_t Caps2;
    uint32_t Caps3;
    uint32_t Caps4;
    uint32_t Reserved2;
};

struct DDSHeaderDXT10_s
{
    rl::RenderFormat Format;
    uint32_t ResourceDimension;
    uint32_t MiscFlag;
    uint32_t ArraySize;
    uint32_t MiscFlags2;
};

#pragma pack(pop)

static_assert(sizeof(DDSPixelFormat_s) == 32, "DDS pixel format size mismatch");
static_assert(sizeof(DDSHeader_s) == 124, "DDS Header size mismatch");
static_assert(sizeof(DDSHeaderDXT10_s) == 20, "DDS DX10 Extended Header size mismatch");

constexpr size_t DDSMinHeaderSize = sizeof(uint32_t) + sizeof(DDSHeader_s);
constexpr size_t DDSDX10HeaderSize = sizeof(uint32_t) + sizeof(DDSHeader_s) + sizeof(DDSHeaderDXT10_s);
static_assert(DDSDX10HeaderSize > DDSMinHeaderSize, "DDS DX10 Header should be larger than standard header");

#define ISBITMASK( r,g,b,a ) ( PixelFormat.RBitMask == r && PixelFormat.GBitMask == g && PixelFormat.BBitMask == b && PixelFormat.ABitMask == a )

rl::RenderFormat GetFormat(const DDSPixelFormat_s& PixelFormat) noexcept
{
    if (PixelFormat.Flags & DDS_RGB)
    {
        // Note that sRGB formats are written using the "DX10" extended header

        switch (PixelFormat.RGBBitCount)
        {
        case 32:
            if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
            {
                return rl::RenderFormat::R8G8B8A8_UNORM;
            }

            if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
            {
                return rl::RenderFormat::B8G8R8A8_UNORM;
            }

            if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0))
            {
                return rl::RenderFormat::B8G8R8X8_UNORM;
            }

            // No DXGI format maps to ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0) aka D3DFMT_X8B8G8R8

            // Note that many common DDS reader/writers (including D3DX) swap the
            // the RED/BLUE masks for 10:10:10:2 formats. We assume
            // below that the 'backwards' header mask is being used since it is most
            // likely written by D3DX. The more robust solution is to use the 'DX10'
            // header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

            // For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
            if (ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
            {
                return rl::RenderFormat::R10G10B10A2_UNORM;
            }

            // No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10

            if (ISBITMASK(0x0000ffff, 0xffff0000, 0, 0))
            {
                return rl::RenderFormat::R16G16_UNORM;
            }

            if (ISBITMASK(0xffffffff, 0, 0, 0))
            {
                // Only 32-bit color channel format in D3D9 was R32F
                return rl::RenderFormat::R32_FLOAT; // D3DX writes this out as a FourCC of 114
            }
            break;

        case 24:
            // No 24bpp DXGI formats aka D3DFMT_R8G8B8
            break;

        case 16:
            if (ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x8000))
            {
                return rl::RenderFormat::B5G5R5A1_UNORM;
            }
            if (ISBITMASK(0xf800, 0x07e0, 0x001f, 0))
            {
                return rl::RenderFormat::B5G6R5_UNORM;
            }

            // No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0) aka D3DFMT_X1R5G5B5

            if (ISBITMASK(0x0f00, 0x00f0, 0x000f, 0xf000))
            {
                return rl::RenderFormat::B4G4R4A4_UNORM;
            }

            // NVTT versions 1.x wrote this as RGB instead of LUMINANCE
            if (ISBITMASK(0x00ff, 0, 0, 0xff00))
            {
                return rl::RenderFormat::R8G8_UNORM;
            }
            if (ISBITMASK(0xffff, 0, 0, 0))
            {
                return rl::RenderFormat::R16_UNORM;
            }

            // No DXGI format maps to ISBITMASK(0x0f00,0x00f0,0x000f,0) aka D3DFMT_X4R4G4B4

            // No 3:3:2:8 or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_A8P8, etc.
            break;

        case 8:
            // NVTT versions 1.x wrote this as RGB instead of LUMINANCE
            if (ISBITMASK(0xff, 0, 0, 0))
            {
                return rl::RenderFormat::R8_UNORM;
            }

            // No 3:3:2 or paletted DXGI formats aka D3DFMT_R3G3B2, D3DFMT_P8
            break;

        default:
            return rl::RenderFormat::UNKNOWN;
        }
    }
    else if (PixelFormat.Flags & DDS_LUMINANCE)
    {
        switch (PixelFormat.RGBBitCount)
        {
        case 16:
            if (ISBITMASK(0xffff, 0, 0, 0))
            {
                return rl::RenderFormat::R16_UNORM; // D3DX10/11 writes this out as DX10 extension
            }
            if (ISBITMASK(0x00ff, 0, 0, 0xff00))
            {
                return rl::RenderFormat::R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
            }
            break;

        case 8:
            if (ISBITMASK(0xff, 0, 0, 0))
            {
                return rl::RenderFormat::R8_UNORM; // D3DX10/11 writes this out as DX10 extension
            }

            // No DXGI format maps to ISBITMASK(0x0f,0,0,0xf0) aka D3DFMT_A4L4

            if (ISBITMASK(0x00ff, 0, 0, 0xff00))
            {
                return rl::RenderFormat::R8G8_UNORM; // Some DDS writers assume the bitcount should be 8 instead of 16
            }
            break;

        default:
            return rl::RenderFormat::UNKNOWN;
        }
    }
    else if (PixelFormat.Flags & DDS_ALPHA)
    {
        if (8 == PixelFormat.RGBBitCount)
        {
            return rl::RenderFormat::A8_UNORM;
        }
    }
    else if (PixelFormat.Flags & DDS_BUMPDUDV)
    {
        switch (PixelFormat.RGBBitCount)
        {
        case 32:
            if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
            {
                return rl::RenderFormat::R8G8B8A8_SNORM; // D3DX10/11 writes this out as DX10 extension
            }
            if (ISBITMASK(0x0000ffff, 0xffff0000, 0, 0))
            {
                return rl::RenderFormat::R16G16_SNORM; // D3DX10/11 writes this out as DX10 extension
            }

            // No DXGI format maps to ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000) aka D3DFMT_A2W10V10U10
            break;

        case 16:
            if (ISBITMASK(0x00ff, 0xff00, 0, 0))
            {
                return rl::RenderFormat::R8G8_SNORM; // D3DX10/11 writes this out as DX10 extension
            }
            break;

        default:
            return rl::RenderFormat::UNKNOWN;
        }

        // No DXGI format maps to DDPF_BUMPLUMINANCE aka D3DFMT_L6V5U5, D3DFMT_X8L8V8U8
    }
    else if (PixelFormat.Flags & DDS_FOURCC)
    {
        if (MAKEFOURCC('D', 'X', 'T', '1') == PixelFormat.FourCC)
        {
            return rl::RenderFormat::BC1_UNORM;
        }
        if (MAKEFOURCC('D', 'X', 'T', '3') == PixelFormat.FourCC)
        {
            return rl::RenderFormat::BC2_UNORM;
        }
        if (MAKEFOURCC('D', 'X', 'T', '5') == PixelFormat.FourCC)
        {
            return rl::RenderFormat::BC3_UNORM;
        }

        // While pre-multiplied alpha isn't directly supported by the DXGI formats,
        // they are basically the same as these BC formats so they can be mapped
        if (MAKEFOURCC('D', 'X', 'T', '2') == PixelFormat.FourCC)
        {
            return rl::RenderFormat::BC2_UNORM;
        }
        if (MAKEFOURCC('D', 'X', 'T', '4') == PixelFormat.FourCC)
        {
            return rl::RenderFormat::BC3_UNORM;
        }

        if (MAKEFOURCC('A', 'T', 'I', '1') == PixelFormat.FourCC)
        {
            return rl::RenderFormat::BC4_UNORM;
        }
        if (MAKEFOURCC('B', 'C', '4', 'U') == PixelFormat.FourCC)
        {
            return rl::RenderFormat::BC4_UNORM;
        }
        if (MAKEFOURCC('B', 'C', '4', 'S') == PixelFormat.FourCC)
        {
            return rl::RenderFormat::BC4_SNORM;
        }

        if (MAKEFOURCC('A', 'T', 'I', '2') == PixelFormat.FourCC)
        {
            return rl::RenderFormat::BC5_UNORM;
        }
        if (MAKEFOURCC('B', 'C', '5', 'U') == PixelFormat.FourCC)
        {
            return rl::RenderFormat::BC5_UNORM;
        }
        if (MAKEFOURCC('B', 'C', '5', 'S') == PixelFormat.FourCC)
        {
            return rl::RenderFormat::BC5_SNORM;
        }

        // BC6H and BC7 are written using the "DX10" extended header

        if (MAKEFOURCC('R', 'G', 'B', 'G') == PixelFormat.FourCC)
        {
            return rl::RenderFormat::R8G8_B8G8_UNORM;
        }
        if (MAKEFOURCC('G', 'R', 'G', 'B') == PixelFormat.FourCC)
        {
            return rl::RenderFormat::G8R8_G8B8_UNORM;
        }

        if (MAKEFOURCC('Y', 'U', 'Y', '2') == PixelFormat.FourCC)
        {
            return rl::RenderFormat::YUY2;
        }

        // Check for D3DFORMAT enums being set here
        switch (PixelFormat.FourCC)
        {
        case 36: // D3DFMT_A16B16G16R16
            return rl::RenderFormat::R16G16B16A16_UNORM;

        case 110: // D3DFMT_Q16W16V16U16
            return rl::RenderFormat::R16G16B16A16_SNORM;

        case 111: // D3DFMT_R16F
            return rl::RenderFormat::R16_FLOAT;

        case 112: // D3DFMT_G16R16F
            return rl::RenderFormat::R16G16_FLOAT;

        case 113: // D3DFMT_A16B16G16R16F
            return rl::RenderFormat::R16G16B16A16_FLOAT;

        case 114: // D3DFMT_R32F
            return rl::RenderFormat::R32_FLOAT;

        case 115: // D3DFMT_G32R32F
            return rl::RenderFormat::R32G32_FLOAT;

        case 116: // D3DFMT_A32B32G32R32F
            return rl::RenderFormat::R32G32B32A32_FLOAT;

            // No DXGI format maps to D3DFMT_CxV8U8

        default:
            return rl::RenderFormat::UNKNOWN;
        }
    }

    return rl::RenderFormat::UNKNOWN;
}

#undef ISBITMASK

bool LoadDDSTexture(const wchar_t* FilePath, DDSTexture_s* Asset)
{
    if (!Asset || !FilePath)
    {
        return RET_INVALID_ARGS;
    }

    File_s File = LoadBinaryFile(FilePath);

    if (!File.Loaded())
    {
        return FAILMSG("LoadDDSTexture: File failed to open (%S)", FilePath);
    }       

    if (File.GetSize() < DDSMinHeaderSize)
    {
        return FAILMSG("LoadDDSTexture: File is too small to be a DDS (%S)", FilePath);
    }

    const uint8_t* FileIter = File.Begin();

    uint32_t Magic;
    memcpy(&Magic, FileIter, sizeof(Magic));

    if (Magic != DDSMagic)
    {
        return FAILMSG("LoadDDSTexture: Not a DDS File (%S)", FilePath);
    }

    FileIter += sizeof(Magic);

    DDSHeader_s Header;
    memcpy(&Header, FileIter, sizeof(Header));

    bool IsDXT10Header = false;
    if ((Header.DDSPixelFormat.Flags & DDS_FOURCC) && (MAKEFOURCC('D', 'X', '1', '0') == Header.DDSPixelFormat.FourCC))
    {
        if (File.GetSize() < DDSDX10HeaderSize)
        {
            return FAILMSG("LoadDDSTexture: File is too small to be a DDS (%S)", FilePath);
        }

        IsDXT10Header = true;
    }

    if (IsDXT10Header)
    {
        return FAILMSG("LoadDDSTexture: DXT10 DDS Textures not supported");
    }

    size_t DataOffset = DDSMinHeaderSize + (IsDXT10Header ? sizeof(DDSHeaderDXT10_s) : 0u);

    const uint8_t* DataIter = File.Begin() + DataOffset;
    size_t DataSize = File.GetSize() - DataOffset;

    std::vector<uint8_t> Data;

    Data.resize(DataSize);
    memcpy(Data.data(), DataIter, DataSize);

    uint32_t Width = Header.Width;
    uint32_t Height = Header.Height;
    uint32_t Depth = Header.Depth;
    uint32_t ArraySize = 1u;
    DDSTexture_s::Dimension_e Dimension = DDSTexture_s::Dimension_e::UNKNOWN;
    rl::RenderFormat Format = rl::RenderFormat::UNKNOWN;
    bool IsCubemap = false;
    
    uint32_t MipCount = Header.MipMapCount > 0 ? Header.MipMapCount : 1;

    if (IsDXT10Header)
    {
        FileIter += sizeof(Header);
        DDSHeaderDXT10_s DDSHeaderDXT10;
        memcpy(&DDSHeaderDXT10, FileIter, sizeof(DDSHeaderDXT10));

        ArraySize = DDSHeaderDXT10.ArraySize;
        if (ArraySize == 0)
        {
            return FAILMSG("LoadDDSTexture: 0 array size (%S)", FilePath);
        }

        if (DDSHeaderDXT10.Format >= rl::RenderFormat::COUNT)
        {
            return FAILMSG("LoadDDSTexture: Unsupported format (%S)", FilePath);
        }

        constexpr uint32_t DDSDimTex1D = 2;
        constexpr uint32_t DDSDimTex2D = 3;
        constexpr uint32_t DDSDimTex3D = 4;

        switch (DDSHeaderDXT10.Format)
        {
        case rl::RenderFormat::NV12:
        case rl::RenderFormat::P010:
        case rl::RenderFormat::P016:
        case rl::RenderFormat::OPAQUE_420:
            if ((DDSHeaderDXT10.ResourceDimension != DDSDimTex2D) || (Width % 2) != 0 || (Height % 2) != 0)
            {
                return FAILMSG("LoadDDSTexture: Unsupported format (%S)", FilePath);
            }
            break;

        case rl::RenderFormat::YUY2:
        case rl::RenderFormat::Y210:
        case rl::RenderFormat::Y216:
            if ((Width % 2) != 0)
            {
                return FAILMSG("LoadDDSTexture: Unsupported format (%S)", FilePath);
            }
            break;

        case rl::RenderFormat::NV11:
            if ((Width % 4) != 0)
            {
                return FAILMSG("LoadDDSTexture: Unsupported format (%S)", FilePath);
            }
            break;

        case rl::RenderFormat::AI44:
        case rl::RenderFormat::IA44:
        case rl::RenderFormat::P8:
        case rl::RenderFormat::A8P8:
            return FAILMSG("LoadDDSTexture: Unsupported format (%S)", FilePath);

        default:
            if (rl::BitsPerPixel(DDSHeaderDXT10.Format) == 0)
            {
                return FAILMSG("LoadDDSTexture: Unsupported format (%S)", FilePath);
            }
        }

        Format = DDSHeaderDXT10.Format;

        switch (DDSHeaderDXT10.ResourceDimension)
        {
        case DDSDimTex1D:
            if ((Header.Flags & DDS_HEIGHT) && Height != 1)
            {
                return FAILMSG("LoadDDSTexture: 1D Textures need a fixed height of 1 (%S)", FilePath);
            }
            Height = Depth = 1u;
            Dimension = DDSTexture_s::Dimension_e::ONEDIM;
            break;

        case DDSDimTex2D:
            if (DDSHeaderDXT10.MiscFlag & 0x4) // RESOURCE_MISC_TEXTURECUBE
            {
                ArraySize *= 6;
                IsCubemap = true;
            }
            Depth = 1;
            Dimension = DDSTexture_s::Dimension_e::TWODIM;
            break;
        case DDSDimTex3D:
            if (!(Header.Flags & DDS_HEADER_FLAGS_VOLUME))
            {
                return FAILMSG("LoadDDSTexture: 3D Texture is not flagged as volume, potentially invalid data (%S)", FilePath);
            }
            if (ArraySize > 1)
            {
                return FAILMSG("LoadDDSTexture: 3D Textures do not support arrays (%S)", FilePath);
            }
            Dimension = DDSTexture_s::Dimension_e::THREEDIM;
            break;
        default:
            return FAILMSG("LoadDDSTexture: Unsupported resource dimension (%S)", FilePath);
        }
    }
    else
    {
        Format = GetFormat(Header.DDSPixelFormat);

        if (Format == rl::RenderFormat::UNKNOWN)
        {
            return FAILMSG("LoadDDSTexture: Unknown texture format (%S)", FilePath);
        }

        if (Header.Flags & DDS_HEADER_FLAGS_VOLUME)
        {
            Dimension = DDSTexture_s::Dimension_e::THREEDIM;
        }
        else
        {
            if (Header.Caps2 & DDS_CUBEMAP)
            {
                if ((Header.Caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
                {
                    return FAILMSG("LoadDDSTexture: Cubemaps require all faces to be defined (%S)", FilePath);
                }

                ArraySize = 6u;
                IsCubemap = true;
            }

            Depth = 1u;
            Dimension = DDSTexture_s::Dimension_e::TWODIM;
        }
    }

    constexpr uint32_t MaxMipLevels = 15u;
    constexpr uint32_t MaxArraySize1D = 2048u;
    constexpr uint32_t MaxDim1D = 16384u;
    constexpr uint32_t MaxArraySize2D = 2048u;
    constexpr uint32_t MaxDim2D = 16384u;
    constexpr uint32_t MaxDimCube = 16384u;
    constexpr uint32_t MaxDim3D = 2048u;

    if (MipCount > MaxMipLevels)
    {
        return FAILMSG("LoadDDSTexture: Mip count must be less than 15 (%S)", FilePath);
    }

    switch (Dimension)
    {
    case DDSTexture_s::Dimension_e::ONEDIM:
        if (ArraySize> MaxArraySize1D || Width > MaxDim1D)
        {
            return FAILMSG("LoadDDSTexture: 1D texture width/depth larger than supported by hardware (%S)", FilePath);
        }
        break;
    case DDSTexture_s::Dimension_e::TWODIM:
        if (IsCubemap)
        {
            if (Depth > MaxArraySize2D || Width > MaxDimCube || Height > MaxDimCube)
            {
                return FAILMSG("LoadDDSTexture: Cube texture width/height/depth larger than supported by hardware (%S)", FilePath);
            }
        }
        else if (Depth > MaxArraySize2D || Width > MaxDim2D || Height > MaxDim2D)
        {
            return FAILMSG("LoadDDSTexture: 2D texture width/height/depth larger than supported by hardware (%S)", FilePath);
        }
        break;
    case DDSTexture_s::Dimension_e::THREEDIM:
        if (Depth > MaxDim3D || Width > MaxDim3D || Height > MaxDim3D)
        {
            return FAILMSG("LoadDDSTexture: 3D texture width/height/depth larger than supported by hardware (%S)", FilePath);
        }
        break;
    default:
        return FAILMSG("LoadDDSTexture: Unsupported texture dimension (%S)", FilePath);
    }

    size_t NumResources = Dimension == DDSTexture_s::Dimension_e::THREEDIM ? 1 : ArraySize;
    NumResources *= MipCount;

    constexpr uint32_t MaxResources = 30720u;

    if (NumResources > MaxResources)
    {
        return FAILMSG("LoadDDSTexture: Resource count too high (%S)", FilePath);
    }

    size_t TexWidth = 0;
    size_t TexHeight = 0;
    size_t TexDepth = 0;
    size_t NumBytes = 0;
    size_t RowBytes = 0;
    size_t NumRowsUnused = 0;

    std::vector<DDSTexture_s::Subresource_s> SubResourceInfos;
    SubResourceInfos.reserve(NumResources);

    uint32_t SrcBitOffset = 0u;
    for (uint32_t ArrayIt = 0; ArrayIt < ArraySize; ArrayIt++)
    {
        uint32_t W = Width;
        uint32_t H = Height;
        uint32_t D = Depth;
        for (uint32_t MipIt = 0; MipIt < MipCount; MipIt++)
        {            
            rl::GetTextureSurfaceInfo(W, H, Format, &NumBytes, &RowBytes, &NumRowsUnused);

            if (NumBytes > UINT32_MAX || RowBytes > UINT32_MAX)
            {
                return RET_ARITHMETIC_OVERFLOW;
            }

            if (!TexWidth)
            {
                TexWidth = W;
                TexHeight = H;
                TexDepth = D;
            }

            DDSTexture_s::Subresource_s Res;
            Res.DataOffset = SrcBitOffset;
            Res.RowPitch = static_cast<uint32_t>(RowBytes);
            Res.SlicePitch = static_cast<uint32_t>(NumBytes);

            SubResourceInfos.emplace_back(Res);

            if ((SrcBitOffset + (NumBytes * D)) > DataSize)
            {
                return FAILMSG("LoadDDSTexture: Reached EOF (%S)", FilePath);
            }

            SrcBitOffset += static_cast<uint32_t>(NumBytes * D);

            W = W >> 1;
            H = H >> 1;
            D = D >> 1;

            if (W == 0) W = 1;
            if (H == 0) H = 1;
            if (D == 0) D = 1;
        }
    }
    
    if (SubResourceInfos.empty())
    {
        return FAILMSG("LoadDDSTexture: Failed to load tex data (%S)", FilePath);
    }

    Asset->Width = Width;
    Asset->Height = Height;
    Asset->DepthOrArraySize = Dimension == DDSTexture_s::Dimension_e::THREEDIM ? Depth : ArraySize;
    Asset->Dimension = Dimension;
    Asset->Format = Format;
    Asset->MipCount = MipCount;
    Asset->Cubemap = IsCubemap;

    std::swap(Asset->SubResourceInfos, SubResourceInfos);
    std::swap(Asset->Data, Data);

    LOGINFO("Loaded texture %S, Width: %d, Height: %d, Depth: %d, Array Size: %d, Mips: %d, Is Cubemap: %d", FilePath, Width, Height, Depth, ArraySize, MipCount, IsCubemap);

    return true;
}
