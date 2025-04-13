#pragma once

#include <string>
#include <vector>

struct HPAssetArgs_s
{
	std::wstring AssetType;
	std::vector<std::wstring> Args;
};

void HPCook(const std::wstring& SourceDir, const std::wstring& OutputDir, const std::vector<HPAssetArgs_s>& Args);