#pragma once

#include <string>
#include <vector>

using HPArgs_t = std::vector<std::wstring>;

struct HPAssetArgs_s
{
	std::wstring AssetType;
	HPArgs_t Args;
};

void HPCook(const std::wstring& SourceDir, const std::wstring& OutputDir, const std::vector<HPAssetArgs_s>& Args);