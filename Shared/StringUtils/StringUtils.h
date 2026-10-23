#pragma once

#include <string>

std::string WideToNarrow(std::wstring_view WideStr);
std::wstring NarrowToWide(std::string_view NarrowStr);
std::wstring Replace(std::wstring_view Str, wchar_t Char, wchar_t With);