#include "Object/SkylightComponent.h"

#include "Assets/TextureManager.h"
#include "Rendering/RenderScene.h"

#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/PathUtils.h>

#include <SurfMath.h>

namespace
{
// Shared by all skylights so a replacement at the same address still reads as changed
uint32_t NextSkylightGeneration = 1;
}

void SkylightComponent_c::OnCreate()
{
	Super::OnCreate();

	if (RenderScene_s* RenderScene = GetRenderScene())
	{
		Registered = RenderScene->RegisterSkylight(&SkylightInfo);
	}
}

void SkylightComponent_c::Deserialize(const JsonValue_s& Data)
{
	Super::Deserialize(Data);

	JsonHelpers::ParseFloat(Data, "LuminanceScale", SkylightInfo.LuminanceScale);

	float YawDegrees = 0.0f;
	if (JsonHelpers::ParseFloat(Data, "YawDegrees", YawDegrees))
	{
		SkylightInfo.Yaw = ConvertToRadians(YawDegrees);
	}

	Path_s EnvironmentMapAssetPath;
	if (JsonHelpers::ParsePath(Data, "EnvironmentMapAssetPath", EnvironmentMapAssetPath))
	{
		SetEnvironmentMap(TextureManager::RequestTexture(EnvironmentMapAssetPath));
	}
}

void SkylightComponent_c::PreDestroy()
{
	if (Registered)
	{
		if (RenderScene_s* RenderScene = GetRenderScene())
		{
			RenderScene->UnregisterSkylight(&SkylightInfo);
		}
		Registered = false;
	}

	Super::PreDestroy();
}

void SkylightComponent_c::SetEnvironmentMap(const std::shared_ptr<Texture_s>& Texture)
{
	SkylightInfo.EnvironmentMap = Texture;
	SkylightInfo.Generation = NextSkylightGeneration++;
}
