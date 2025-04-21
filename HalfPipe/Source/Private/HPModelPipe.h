#pragma once

#include "HPPipe.h"

class HPModelPipe_c : public IHPPipe_c
{
public:
	const wchar_t* GetAssetType() const override { return L"Model"; }
	void Cook(const std::wstring& SourceDir, const std::wstring& OutputDir, const HPArgs_t& Args) override;
	std::wstring GetCookedAssetPath(const std::wstring& OutputDir, const HPArgs_t& Args) const;
	std::wstring GetPackageAssetPath(const HPArgs_t& Args) const override;
};
