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

    if (fseek(File, 0, SEEK_END) == 0u)
    {
        return 0u;
    }

    long Size = ftell(File);

    if (Size == -1)
    {
        return 0u;
    }

    if (fseek(File, Offset, SEEK_SET) == 0u)
    {
        return 0u;
    }

    return Size;
}

bool LoadBinaryFile(const char* Path, File_s& OutFile)
{
    FILE* File = nullptr;

    if (!fopen_s(&File, Path, "rb"))
    {
        return false;
    }

    if (!File)
    {
        return false;
    }

    long FileSize = GetFileSize(File);

    fclose(File);
}
