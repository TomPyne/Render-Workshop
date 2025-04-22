#include "HPModel.h"
#include "HPModel.h"

#include <FileUtils/FileStream.h>
#include <Logging/Logging.h>

bool HPModel_s::Mesh_s::Serialize(FileStream_s& Stream)
{
    Stream.Stream(&IndexOffset);
    Stream.Stream(&IndexCount);
    Stream.Stream(&MeshletOffset);
    Stream.Stream(&MeshletCount);
    Stream.StreamStr(&LibMaterialName);

    return true;
}

bool HPModel_s::Serialize(const std::wstring& Path, FileStreamMode_e Mode)
{
    FileStream_s Stream(Path, Mode);

    if (!Stream.IsOpen())
    {
        return false;
    }

    return Serialize(Stream);
}

bool HPModel_s::Serialize(FileStream_s& Stream)
{
    CHECK(Stream.IsOpen());

    Stream.Stream(&Version);

    if (Version != HPModelVersion_e::CURRENT)
    {
        LOGWARNING("[HPModel] Serializing with version mismatch");

        return false;
    }

    Stream.Stream(&VertexCount);
    Stream.Stream(&IndexCount);
    Stream.Stream(&HasNormals);
    Stream.Stream(&HasTangents);
    Stream.Stream(&HasBitangents);
    Stream.Stream(&HasTexcoords);
    Stream.Stream(&IndexFormat);

    Stream.Stream(&Positions, VertexCount);

    if (HasNormals)
    {
        Stream.Stream(&Normals, VertexCount);
    }

    if (HasTangents)
    {
        Stream.Stream(&Tangents, VertexCount);
    }

    if (HasBitangents)
    {
        Stream.Stream(&Bitangents, VertexCount);
    }

    if (HasTexcoords)
    {
        Stream.Stream(&Texcoords, VertexCount);
    }

    size_t IndexBufByteCount = IndexFormat == rl::RenderFormat::R32_UINT ? 4 * IndexCount : 2 * IndexCount;

    Stream.Stream(&Indices, static_cast<uint32_t>(IndexBufByteCount));

    size_t MeshCount = Meshes.size();
    Stream.Stream(&MeshCount);
    Meshes.resize(MeshCount);

    for (Mesh_s& Mesh : Meshes)
    {
        if (!Mesh.Serialize(Stream))
        {
            return false;
        }
    }

    Stream.Stream(&Meshlets);
    Stream.Stream(&UniqueVertexIndices);
    Stream.Stream(&PrimitiveIndices);

    Stream.StreamStr(&MaterialLibPath);
    Stream.StreamStr(&SourcePath);

    return true;
}
