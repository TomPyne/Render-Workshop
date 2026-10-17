#include "Rendering/RenderScene.h"

#include <Shared/Logging/Logging.h>

void RenderScene_s::RegisterRenderable(IRenderable_c* Renderable)
{
	if (Renderable)
	{
		RenderableComponents.push_back(Renderable);
		DirtyRenderScene();
	}
}

void RenderScene_s::UnregisterRenderable(IRenderable_c * Renderable)
{
	if (Renderable)
	{
		std::erase(RenderableComponents, Renderable);
		DirtyRenderScene();
	}
}

void RenderScene_s::DirtyRenderScene()
{
	RenderSceneDirty = true;
}

void RenderScene_s::RegisterParticleSystem(const ParticleSystemInfo_s* ParticleSystemInfo)
{
	if (ParticleSystemInfo)
	{
		ParticleSystems.push_back(ParticleSystemInfo);
	}
}

void RenderScene_s::UnregisterParticleSystem(const ParticleSystemInfo_s* ParticleSystemInfo)
{
	if (ParticleSystemInfo)
	{
		std::erase(ParticleSystems, ParticleSystemInfo);
	}
}
bool RenderScene_s::RegisterSkylight(const SkylightInfo_s* SkylightInfo)
{
	if (!SkylightInfo)
		return false;

	if (Skylight)
	{
		LOGWARNING("[RenderScene] Only one skylight is supported, ignoring the new one");
		return false;
	}

	Skylight = SkylightInfo;
	return true;
}

void RenderScene_s::UnregisterSkylight(const SkylightInfo_s* SkylightInfo)
{
	if (SkylightInfo && Skylight == SkylightInfo)
	{
		Skylight = nullptr;
	}
}

bool RenderScene_s::RegisterDirectionalLight(DirectionalLightInfo_s* DirectionalLightInfo)
{
	if (!DirectionalLightInfo)
		return false;

	if (DirectionalLight)
	{
		LOGWARNING("[RenderScene] Only one directional light is supported, ignoring the new one");
		return false;
	}

	DirectionalLight = DirectionalLightInfo;
	return true;
}

void RenderScene_s::UnregisterDirectionalLight(const DirectionalLightInfo_s* DirectionalLightInfo)
{
	if (DirectionalLightInfo && DirectionalLight == DirectionalLightInfo)
	{
		DirectionalLight = nullptr;
	}
}

bool RenderScene_s::RegisterHeightFog(HeightFogInfo_s* HeightFogInfo)
{
	if (!HeightFogInfo)
		return false;

	if (HeightFog)
	{
		LOGWARNING("[RenderScene] Only one height fog is supported, ignoring the new one");
		return false;
	}

	HeightFog = HeightFogInfo;
	return true;
}

void RenderScene_s::UnregisterHeightFog(const HeightFogInfo_s* HeightFogInfo)
{
	if (HeightFogInfo && HeightFog == HeightFogInfo)
	{
		HeightFog = nullptr;
	}
}
