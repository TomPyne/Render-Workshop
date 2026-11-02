#include "Object/HeightFogComponent.h"

#include "Object/PropertyVisitor.h"
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

void HeightFogComponent_c::VisitProperties(PropertyVisitor_c& Visitor)
{
	Super::VisitProperties(Visitor);

	Visitor.Bool("Enabled", HeightFogInfo.Enabled);
	Visitor.Float("Density", HeightFogInfo.Density, { .Min = 0.0f, .Max = 1.0f, .Speed = 0.0001f });
	Visitor.Float("HeightFalloff", HeightFogInfo.HeightFalloff, { .Min = 0.0f, .Max = 10.0f, .Speed = 0.001f });
	Visitor.Float("StartDistance", HeightFogInfo.StartDistance, { .Min = 0.0f, .Max = 100'000.0f, .Speed = 1.0f });
	Visitor.Float("MaxOpacity", HeightFogInfo.MaxOpacity, { .Min = 0.0f, .Max = 1.0f, .Speed = 0.01f });
	Visitor.Float("SkyDistance", HeightFogInfo.SkyDistance, { .Min = 0.0f, .Max = 1'000'000.0f, .Speed = 10.0f });
	Visitor.Color3("Albedo", HeightFogInfo.Albedo);
	Visitor.Float("SunAnisotropy", HeightFogInfo.SunAnisotropy, { .Min = -0.99f, .Max = 0.99f, .Speed = 0.01f });
	Visitor.Float("SunScatterScale", HeightFogInfo.SunScatterScale, { .Min = 0.0f, .Max = 100.0f, .Speed = 0.01f });
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
