#include "HPTexture.h"

#include <FileUtils/FileStream.h>
#include <Logging/Logging.h>

bool HPTexture_s::Serialize(const std::wstring& Path, FileStreamMode_e Mode)
{
    FileStream_s Stream(Path, Mode);

    if (!Stream.IsOpen())
    {
        return false;
    }

    Stream.Stream(&Version);

    if (Version != HPTextureVersion_e::CURRENT)
    {
        LOGWARNING("[HPTexture] Serializing with version mismatch");

        return false;
    }

    Stream.Stream(&Width);
    Stream.Stream(&Height);
    Stream.Stream(&Format);
    Stream.Stream(&Mips);
    Stream.Stream(&Data);
    Stream.StreamStr(&SourcePath);

    return true;
}
