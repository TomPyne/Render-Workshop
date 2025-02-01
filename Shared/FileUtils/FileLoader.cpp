#include "FileLoader.h"

long GetFileSize(FILE* File)
{
    if (!File)
    {
        return 0u;
    }

    long Offset = ftell(File);
    if (Offset == -1)
    {
        return 0u;
    }

    if (fseek(File, 0, SEEK_END) != 0u)
    {
        return 0u;
    }

    long Size = ftell(File);

    if (Size == -1)
    {
        return 0u;
    }

    if (fseek(File, Offset, SEEK_SET) != 0u)
    {
        return 0u;
    }

    return Size;
}

File_s LoadBinaryFile(FILE* File)
{
    if (!File)
    {
        return {};
    }

    File_s LoadedFile;

    long FileSize = GetFileSize(File);

    if (FileSize > 0)
    {
        LoadedFile.Bytes.resize(static_cast<size_t>(FileSize));

        fread_s(LoadedFile.Bytes.data(), LoadedFile.Bytes.size(), sizeof(uint8_t), LoadedFile.Bytes.size(), File);
    }

    fclose(File);

    return LoadedFile;
}

File_s LoadBinaryFile(const char* Path)
{   
    FILE* File = nullptr;

    if (fopen_s(&File, Path, "rb") != 0)
    {
        return {};
    }

    return LoadBinaryFile(File);
}

File_s LoadBinaryFile(const wchar_t* Path)
{
    FILE* File = nullptr;

    if (_wfopen_s(&File, Path, L"rb") != 0)
    {
        return {};
    }

    return LoadBinaryFile(File);
}
