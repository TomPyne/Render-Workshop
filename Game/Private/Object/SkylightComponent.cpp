#include "Object/SkylightComponent.h"

#include "Assets/TextureManager.h"
#include "Object/PropertyVisitor.h"
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

void SkylightComponent_c::VisitProperties(PropertyVisitor_c& Visitor)
{
	Super::VisitProperties(Visitor);

	Visitor.Float("LuminanceScale", SkylightInfo.LuminanceScale, { .Min = 0.0f, .Max = 100'000.0f, .Speed = 10.0f });
	Visitor.Angle("YawDegrees", SkylightInfo.Yaw, { .Min = -180.0f, .Max = 180.0f, .Speed = 0.5f });

	if (Visitor.Path("EnvironmentMapAssetPath", EnvironmentMapAssetPath))
	{
		SetEnvironmentMap(TextureManager::RequestTexture(Path_s::FromTokenised(EnvironmentMapAssetPath)));
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
