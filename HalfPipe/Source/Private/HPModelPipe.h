#pragma once

#include "HPPipe.h"

class HPModelPipe_c : public IHPPipe_c
{
public:
	const wchar_t* GetAssetType() const override { return L"Model"; }
	void Cook(const std::wstring& SourcePath, const std::wstring& OutputPath, const std::vector<std::wstring>& Args) override;
};
