#pragma once

struct AssetConfig_s
{
	bool SkipCookedLoading = false;
	bool SkipCookedWriting = false;
};

extern AssetConfig_s GAssetConfig;