#include "Rendering/RenderScene.h"

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
{}

void RenderScene_s::UnregisterParticleSystem(const ParticleSystemInfo_s * ParticleSystemInfo)
{}
