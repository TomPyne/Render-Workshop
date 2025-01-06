#pragma once

#include "FileUtils/PathUtils.h"

#include <SurfMath.h>
#include <Render/RenderTypes.h>
#include <Render/Buffers.h>

#include <string>

struct MaterialAsset_s
{
	float3 Albedo;
	wchar_t AlbedoTexturePath[PathUtils::MaxPath];

	std::wstring SourcePath;
};

MaterialAsset_s* LoadMaterialAsset(const std::wstring& FilePath);
void WriteMaterialAsset(const std::wstring& FilePath, const MaterialAsset_s* const Asset);