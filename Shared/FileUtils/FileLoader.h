#pragma once

#include <vector>

struct File_s
{
	std::vector<uint8_t> Bytes;
};

bool LoadBinaryFile(const char* Path, File_s& OutFile);