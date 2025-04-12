#pragma once

#include "FileUtils/PathUtils.h"

struct ModelAsset_s;

bool CookModel(const std::wstring& AssetPath, ModelAsset_s** OutAsset = nullptr);
ModelAsset_s* LoadModel(const std::wstring& Path);
