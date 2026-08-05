#include "Assets/MaterialManager.h"

#include "Rendering/Materials.h"
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

#include <Render/RenderTypes.h>

#define MATERIAL_ASSET_VERSION_INITIAL 1
#define MATERIAL_ASSET_VERSION_CURRENT MATERIAL_ASSET_VERSION_INITIAL

#define SHADER_ASSET_VERSION_INITIAL 1
#define SHADER_ASSET_VERSION_CURRENT SHADER_ASSET_VERSION_INITIAL

namespace MaterialManager
{

std::shared_ptr<Material_s> RequestMaterial(const Path_s& Path)
{
	Json_t Json;
	if (ENSUREMSG(LoadJsonFromFile(Path.ToWString(), Json), "[MaterialManager::RequestMaterial] Failed to load Material json from path %S", Path.ToWString().c_str()))
	{
		return RequestMaterial(Json);
	}
	return nullptr;
}

std::shared_ptr<Material_s> RequestMaterial(const JsonValue_s& Data)
{
	int32_t Version = -1;
	JsonHelpers::ParseInt(Data, "Version", Version);
	if (!ENSUREMSG(Version == MATERIAL_ASSET_VERSION_CURRENT, "[MaterialManager::RequestMaterial] Unsupported material asset version: %d, curremt: %d", Version, MATERIAL_ASSET_VERSION_CURRENT))
	{
		return nullptr;
	}

	std::wstring ShaderAssetPath;
	if (!ENSUREMSG(JsonHelpers::ParseWString(Data, "ShaderAssetPath", ShaderAssetPath), "[MaterialManager::RequestMaterial] No ShaderAssetPath supplied"))
	{
		return nullptr;
	}

	float3 Color = float3(0.5f);
	JsonHelpers::ParseFloat3(Data, "Color", Color);

	CHECK(false); // TODO
	return nullptr; 
}

std::shared_ptr<Shader_s> RequestShader(const Path_s& Path)
{
	Json_t Json;
	if (ENSUREMSG(LoadJsonFromFile(Path.ToWString(), Json), "[MaterialManager::RequestShader] Failed to load Shader json from path %S", Path.ToWString().c_str()))
	{
		return RequestShader(Json);
	}
	return nullptr;
}

std::shared_ptr<Shader_s> RequestShader(const JsonValue_s& Data)
{
	int32_t Version = -1;
	JsonHelpers::ParseInt(Data, "Version", Version);
	if (!ENSUREMSG(Version == SHADER_ASSET_VERSION_INITIAL, "[MaterialManager::RequestShader] Unsupported shader asset version: %d, curremt: %d", Version, SHADER_ASSET_VERSION_CURRENT))
	{
		return nullptr;
	}

	CHECK(false); // TODO
	return nullptr;
}

}