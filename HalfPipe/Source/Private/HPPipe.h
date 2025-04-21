#pragma once

#include "HalfPipe.h"

#include <string>
#include <vector>

// TODO rather than exporting with cook we should gather all cooked assets and write in one pass to ensure there
// are no conflicts on output. For now I will just be careful with my asset set ups until it inevitably catches me out

class IHPPipe_c
{
public:
	virtual const wchar_t* GetAssetType() const = 0;
	virtual void Cook(const std::wstring& SourceDir, const std::wstring& OutputDir, const std::vector<std::wstring>& Args) = 0;
	virtual std::wstring GetCookedAssetPath(const std::wstring& OutputDir, const std::vector<std::wstring>& Args) const = 0;

protected:

	static bool ParseArgs(const std::vector<std::wstring>& Args, const wchar_t* Command, std::wstring& OutValue);
};

void InitPipes();
IHPPipe_c* GetPipeForAsset(const wchar_t* AssetType);
std::wstring GetCookedPathForAssetFromArgs(const std::wstring& OutputDir, const HPAssetArgs_s& AssetArgs);

void PushCookCommand(const HPAssetArgs_s& Args);
void ProcessCookCommands(const std::wstring& SourceDir, const std::wstring& OutputDir);
