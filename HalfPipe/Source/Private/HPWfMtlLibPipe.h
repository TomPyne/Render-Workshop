#pragma once

#include "HPPipe.h"

class HPWfMtlLibPipe_c : public IHPPipe_c
{
public:
	const wchar_t* GetAssetType() const override { return L"WfMtlLib"; }
	void Cook(const std::wstring& SourceDir, const std::wstring& OutputDir, const std::vector<std::wstring>& Args) override;
	std::wstring GetCookedAssetPath(const std::wstring& OutputDir, const std::vector<std::wstring>& Args) const override;
};