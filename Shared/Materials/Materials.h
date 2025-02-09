#pragma once

#include <SurfMath.h>
#include <string>

struct MaterialAsset_s
{
	float3 Albedo;
	float Metallic;
	float Roughness;
	std::wstring AlbedoTexture;
	std::wstring NormalTexture;
	std::wstring MetallicTexture;
	std::wstring RoughnessTexture;

	std::wstring SourcePath;
};

MaterialAsset_s* LoadMaterialAsset(const std::wstring& FilePath);
void WriteMaterialAsset(const std::wstring& FilePath, MaterialAsset_s* Asset);