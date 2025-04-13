#include "HPModel.h"

bool HPModel_s::Serialize(const std::wstring& Path, FileStreamMode_e Mode)
{
    FileStream_s Stream(Path, Mode);

    if (!Stream.IsOpen())
    {
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

    size_t IndexBufByteCount = IndexFormat == tpr::RenderFormat::R32_UINT ? 4 * IndexCount : 2 * IndexCount;

    Stream.Stream(&Indices, IndexBufByteCount);

    Stream.Stream(&Meshes);
    Stream.Stream(&Meshlets);
    Stream.Stream(&UniqueVertexIndices);
    Stream.Stream(&PrimitiveIndices);

    return true;
}
