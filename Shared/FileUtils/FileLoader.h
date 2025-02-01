#pragma once

#include <vector>

struct File_s
{
	std::vector<uint8_t> Bytes;

	inline bool Loaded() const noexcept { return !Bytes.empty(); }
	inline size_t GetSize() const noexcept { return Bytes.size(); }
	const uint8_t* Begin() const noexcept { return Bytes.empty() ? nullptr : &Bytes.front(); }
};

File_s LoadBinaryFile(const char* Path);
File_s LoadBinaryFile(const wchar_t* Path);