#include "HPWfMtlLib.h"

#include <FileUtils/FileStream.h>
#include <Logging/Logging.h>

bool HPWfMtlLib_s::Serialize(const std::wstring& Path, FileStreamMode_e Mode)
{
    FileStream_s Stream(Path, Mode);

    if (!Stream.IsOpen())
    {
        return false;
    }

    Stream.Stream(&Version);

    if (Version != HPWfMtlLibVersion_e::CURRENT)
    {
        LOGWARNING("[HPWfMtlLib] Serializing with version mismatch");

        return false;
    }

    size_t MaterialCount = Materials.size();
    Stream.Stream(&MaterialCount);

    if (Materials.size() != MaterialCount)
    {
        Materials.resize(MaterialCount);
    }

    for (size_t MaterialIt = 0; MaterialIt < MaterialCount; MaterialIt++)
    {
        Stream.StreamStr(&Materials[MaterialIt].DiffuseTexture);
        Stream.StreamStr(&Materials[MaterialIt].SpecularTexture);
        Stream.StreamStr(&Materials[MaterialIt].NormalTexture);
    }

    Stream.StreamStr(&SourcePath);

    return true;
}
