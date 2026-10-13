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
