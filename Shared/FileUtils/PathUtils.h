#pragma once

#include <string>

namespace PathUtils
{
	static constexpr size_t MaxPath = 260;
}

std::wstring GetPathExtension(const wchar_t* Path);
std::wstring ReplacePathExtension(const wchar_t* Path, const wchar_t* NewExtension);
bool HasPathExtension(const wchar_t* Path, const wchar_t* Extension);

inline std::wstring GetPathExtension(const std::wstring& Path) { return GetPathExtension(Path.c_str()); }
inline std::wstring ReplacePathExtension(const std::wstring& Path, const wchar_t* NewExtension) { return ReplacePathExtension(Path.c_str(), NewExtension); }
inline bool HasPathExtension(const std::wstring& Path, const wchar_t* Extension) { return HasPathExtension(Path.c_str(), Extension); }