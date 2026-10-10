#include "Object/SkylightComponent.h"

#include "Assets/TextureManager.h"

#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/PathUtils.h>

void SkylightComponent::OnCreate()
{
	Super::OnCreate();
}

void SkylightComponent::Deserialize(const JsonValue_s& Data)
{
	Super::Deserialize(Data);

	Path_s EnvironmentMapAssetPath;
	if (JsonHelpers::ParsePath(Data, "EnvironmentMapAssetPath", EnvironmentMapAssetPath))
	{
		SetEnvironmentMap(TextureManager::RequestTexture(EnvironmentMapAssetPath));
	}
}

void SkylightComponent::PreDestroy()
{

	Super::PreDestroy();
}

void SkylightComponent::SetEnvironmentMap(const std::shared_ptr<Texture_s>& Texture)
{
	EnvironmentMap = Texture;
}
