#pragma once

#include "HalfPipe.h"

#include <string>
#include <vector>

class IHPPipe_c
{
public:
	virtual const wchar_t* GetAssetType() const = 0;
	virtual void Cook(const std::wstring& SourceDir, const std::wstring& OutputDir, const std::vector<std::wstring>& Args) = 0;
};

void InitPipes();
IHPPipe_c* GetPipeForAsset(const wchar_t* AssetType);
