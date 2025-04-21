#pragma once

#include "HPPipe.h"

class HPTexturePipe_c : public IHPPipe_c
{
public:
	const wchar_t* GetAssetType() const override { return L"Texture"; }
	void Cook(const std::wstring& SourceDir, const std::wstring& OutputDir, const HPArgs_t& Args) override;
	std::wstring GetCookedAssetPath(const std::wstring& OutputDir, const HPArgs_t& Args) const;
	std::wstring GetPackageAssetPath(const HPArgs_t& Args) const override;
};