#include "Object/HeightFogComponent.h"

#include "Rendering/RenderScene.h"

#include <Shared/FileUtils/JsonValue.h>

void HeightFogComponent_c::OnCreate()
{
	Super::OnCreate();

	UpdateBaseHeight();

	if (RenderScene_s* RenderScene = GetRenderScene())
	{
		Registered = RenderScene->RegisterHeightFog(&HeightFogInfo);
	}
}

void HeightFogComponent_c::Deserialize(const JsonValue_s& Data)
{
	Super::Deserialize(Data);

	JsonHelpers::ParseBool(Data, "Enabled", HeightFogInfo.Enabled);
	JsonHelpers::ParseFloat(Data, "Density", HeightFogInfo.Density);
	JsonHelpers::ParseFloat(Data, "HeightFalloff", HeightFogInfo.HeightFalloff);
	JsonHelpers::ParseFloat(Data, "StartDistance", HeightFogInfo.StartDistance);
	JsonHelpers::ParseFloat(Data, "MaxOpacity", HeightFogInfo.MaxOpacity);
	JsonHelpers::ParseFloat(Data, "SkyDistance", HeightFogInfo.SkyDistance);
	JsonHelpers::ParseFloat3(Data, "Albedo", HeightFogInfo.Albedo);
	JsonHelpers::ParseFloat(Data, "SunAnisotropy", HeightFogInfo.SunAnisotropy);
	JsonHelpers::ParseFloat(Data, "SunScatterScale", HeightFogInfo.SunScatterScale);
}

void HeightFogComponent_c::PreDestroy()
{
	if (Registered)
	{
		if (RenderScene_s* RenderScene = GetRenderScene())
		{
			RenderScene->UnregisterHeightFog(&HeightFogInfo);
		}
		Registered = false;
	}

	Super::PreDestroy();
}

void HeightFogComponent_c::OnTransformed()
{
	Super::OnTransformed();

	UpdateBaseHeight();
}

void HeightFogComponent_c::UpdateBaseHeight()
{
	HeightFogInfo.BaseHeight = GetWorldPosition().y;
}
