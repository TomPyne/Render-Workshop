#pragma once

#include <string>

std::string WideToNarrow(const std::wstring& WideStr);
std::wstring NarrowToWide(const std::string& NarrowStr);
std::wstring Replace(const std::wstring& Str, wchar_t Char, wchar_t With);