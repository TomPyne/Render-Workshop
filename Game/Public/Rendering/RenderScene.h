#pragma once

#include "Rendering/DistanceFieldScene.h"

#include <Render/Raytracing.h>
#include <Render/RenderTypes.h>
#include <RenderUtils/FrameBuffer/FrameBufferAlloc.h>
#include <Shared/Types/Enum.h>

#include <vector>

struct RenderScene_s
{
	bool RenderSceneDirty = false;
	void DirtyRenderScene();

	// Rendering //////////////////////////////////////////////////////////////////////////////////
	void RegisterRenderable(class IRenderable_c* Renderable);
	void UnregisterRenderable(class IRenderable_c* Renderable);
	const std::vector<class IRenderable_c*>& GetRenderables() const { return RenderableComponents; }

	// Particles //////////////////////////////////////////////////////////////////////////////////
	void RegisterParticleSystem(const struct ParticleSystemInfo_s* ParticleSystemInfo);
	void UnregisterParticleSystem(const struct ParticleSystemInfo_s* ParticleSystemInfo);
	const std::vector<const struct ParticleSystemInfo_s*>& GetParticleSystems() const { return ParticleSystems; }

private:
	std::vector<class IRenderable_c*> RenderableComponents;
	std::vector<const struct ParticleSystemInfo_s*> ParticleSystems;
};