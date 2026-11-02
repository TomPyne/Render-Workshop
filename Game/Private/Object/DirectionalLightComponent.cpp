#include "Object/DirectionalLightComponent.h"

#include "Object/PropertyVisitor.h"
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

void DirectionalLightComponent_c::VisitProperties(PropertyVisitor_c& Visitor)
{
	Super::VisitProperties(Visitor);

	Visitor.Float("IlluminanceLux", DirectionalLightInfo.IlluminanceLux, { .Min = 0.0f, .Max = 200'000.0f, .Speed = 100.0f });
	Visitor.Color3("Color", DirectionalLightInfo.Color);
	Visitor.Angle("SoftAngleDegrees", DirectionalLightInfo.SoftAngle, { .Min = 0.0f, .Max = 10.0f, .Speed = 0.01f });
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
