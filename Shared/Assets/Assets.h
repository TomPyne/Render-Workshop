#pragma once

#include <map>
#include <shared_mutex>
#include <string>

struct AssetConfig_s
{
	bool CookOnDemand = true;
	bool SkipCookedLoading = false; // Remove this
	bool SkipCookedWriting = false;
};

extern AssetConfig_s GAssetConfig;

template<typename T>
struct AssetLibrary_s
{
	T* FindAsset(const std::wstring& Path)
	{
		auto ReadLock = std::shared_lock(Mutex);

		auto Found = LoadedAssets.find(Path);
		if (Found != LoadedAssets.end())
		{
			return Found->second.get();
		}

		return nullptr;
	}

	T* CreateAsset(const std::wstring& Path, T* Asset = nullptr)
	{
		auto WriteLock = std::unique_lock(Mutex);

		if (!Asset)
		{
			Asset = new T;
		}

		LoadedAssets.emplace(Path, Asset);

		return Asset;
	}

private:
	std::map<std::wstring, std::unique_ptr<T>> LoadedAssets;

	std::shared_mutex Mutex;
};