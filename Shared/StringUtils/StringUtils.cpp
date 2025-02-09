#include "StringUtils.h"

#include <algorithm>

std::string WideToNarrow(const std::wstring& WideStr)
{
    std::string NarrowStr;
    NarrowStr.resize(WideStr.length());

    std::transform(WideStr.begin(), WideStr.end(), NarrowStr.begin(), [](wchar_t WideChar)
    {
        return (char)WideChar;
    });

    return NarrowStr;
}

std::wstring NarrowToWide(const std::string& NarrowStr)
{
    std::wstring WideStr;
    WideStr.resize(NarrowStr.length());

    std::transform(NarrowStr.begin(), NarrowStr.end(), WideStr.begin(), [](char NarrowChar)
    {
        return (wchar_t)NarrowChar;
    });

    return WideStr;
}

std::wstring Replace(const std::wstring& Str, wchar_t Char, wchar_t With)
{
    std::wstring Ret = Str;

    std::transform(Str.begin(), Str.end(), Ret.begin(), [Char, With](wchar_t C)
    {
        return C == Char ? With : C;
    });

    return Ret;
}
