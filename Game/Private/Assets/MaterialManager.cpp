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
struct MaterialManagerGlobals_s
{
	std::unordered_map<std::wstring, std::shared_ptr<MaterialShader_c>> RegisteredMaterials;
	std::unordered_map<uint64_t, std::shared_ptr<MaterialShaderInstance_c>> LoadedMaterialInstances;
} G;

std::shared_ptr<MaterialShaderInstance_c> RequestMaterialInstance(const Path_s& Path)
{
	Json_t Json;
	if (ENSUREMSG(LoadJsonFromFile(Path.ToWString(), Json), "[MaterialManager::RequestMaterialInstance] Failed to load Material json from path %S", Path.ToWString().c_str()))
	{
		return RequestMaterialInstance(Json);
	}
	return nullptr;
}

std::shared_ptr<MaterialShaderInstance_c> RequestMaterialInstance(const JsonValue_s& Data)
{
	// Compute hash of data to use as a key for caching
	auto It = G.LoadedMaterialInstances.find(Data.GetHash());
	if (It != G.LoadedMaterialInstances.end())
	{
		return It->second;
	}

	int32_t Version = -1;
	JsonHelpers::ParseInt(Data, "Version", Version);
	if (!ENSUREMSG(Version == MATERIAL_ASSET_VERSION_CURRENT, "[MaterialManager::RequestMaterialInstance] Unsupported material asset version: %d, curremt: %d", Version, MATERIAL_ASSET_VERSION_CURRENT))
	{
		return nullptr;
	}

	std::wstring MaterialShaderClass;
	if (!ENSUREMSG(JsonHelpers::ParseWString(Data, "MaterialShaderClass", MaterialShaderClass), "[MaterialManager::RequestMaterialInstance] No MaterialShaderClass supplied"))
	{
		return nullptr;
	}

	std::shared_ptr<MaterialShader_c> Parent;
	auto FoundMaterialIt = G.RegisteredMaterials.find(MaterialShaderClass);
	if (FoundMaterialIt == G.RegisteredMaterials.end())
	{
		std::shared_ptr<MaterialShader_c> NewMaterialShader = MaterialShaderFactory_s::Get().CreateShaderMaterial(MaterialShaderClass);
		if (ENSUREMSG(NewMaterialShader != nullptr, "[MaterialManager::RequestMaterialInstance] No valid MaterialShaderClass found %S", MaterialShaderClass.c_str()))
		{
			LOGINFO("[MaterialManager::RequestMaterialInstance] Loading Material Shader: %S", MaterialShaderClass.c_str());
			Parent = NewMaterialShader;
			G.RegisteredMaterials[MaterialShaderClass] = NewMaterialShader;
		}
	}
	else
	{
		Parent = FoundMaterialIt->second;
	}

	if (!ENSUREMSG(Parent != nullptr, "[MaterialManager::RequestMaterialInstance] No valid parent found %S", MaterialShaderClass.c_str()))
	{
		return nullptr;
	}

	std::shared_ptr<MaterialShaderInstance_c> NewMaterialInstance = std::make_shared<MaterialShaderInstance_c>();

	NewMaterialInstance->SetParent(Parent);

	NewMaterialInstance->Deserialize(Data);

	NewMaterialInstance->Update();

	G.LoadedMaterialInstances[Data.GetHash()] = NewMaterialInstance;

	return NewMaterialInstance;
}

MaterialShaderFactory_s& MaterialShaderFactory_s::Get()
{
	static MaterialShaderFactory_s MaterialShaderFactory;
	return MaterialShaderFactory;
}

std::shared_ptr<MaterialShader_c> MaterialShaderFactory_s::CreateShaderMaterial(const std::wstring& ClassName)
{
	auto It = MaterialShaderFactoryCallbacks.find(ClassName);
	if (!ENSUREMSG(It != MaterialShaderFactoryCallbacks.end(), "No material shader class registered for name %S", ClassName.c_str()))
	{
		return nullptr;
	}

	std::shared_ptr<MaterialShader_c> NewMaterialShader = It->second();
	if (NewMaterialShader)
	{
		NewMaterialShader->Load();
		NewMaterialShader->Compile();

		return NewMaterialShader;
	}

	return nullptr;
}

}