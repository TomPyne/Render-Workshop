#include "Object/DirectionalLightComponent.h"

#include "Rendering/RenderScene.h"

#include <Shared/FileUtils/JsonValue.h>

void DirectionalLightComponent_c::OnCreate()
{
	Super::OnCreate();

	UpdateDirection();

	if (RenderScene_s* RenderScene = GetRenderScene())
	{
		Registered = RenderScene->RegisterDirectionalLight(&DirectionalLightInfo);
	}
}

void DirectionalLightComponent_c::Deserialize(const JsonValue_s& Data)
{
	Super::Deserialize(Data);

	JsonHelpers::ParseFloat(Data, "IlluminanceLux", DirectionalLightInfo.IlluminanceLux);
	JsonHelpers::ParseFloat3(Data, "Color", DirectionalLightInfo.Color);

	float SoftAngleDegrees = 0.0f;
	if (JsonHelpers::ParseFloat(Data, "SoftAngleDegrees", SoftAngleDegrees))
	{
		DirectionalLightInfo.SoftAngle = ConvertToRadians(SoftAngleDegrees);
	}
}

void DirectionalLightComponent_c::PreDestroy()
{
	if (Registered)
	{
		if (RenderScene_s* RenderScene = GetRenderScene())
		{
			RenderScene->UnregisterDirectionalLight(&DirectionalLightInfo);
		}
		Registered = false;
	}

	Super::PreDestroy();
}

void DirectionalLightComponent_c::OnTransformed()
{
	Super::OnTransformed();

	UpdateDirection();
}

void DirectionalLightComponent_c::UpdateDirection()
{
	DirectionalLightInfo.Direction = -Normalize(GetWorldForward());
}
