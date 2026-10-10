#pragma once

#include <vector>

struct RenderScene_s
{
	// Rendering //////////////////////////////////////////////////////////////////////////////////
	void RegisterRenderable(IRenderable_c* Renderable);
	void UnregisterRenderable(IRenderable_c* Renderable);
	void DirtyRenderScene();

	// Particles //////////////////////////////////////////////////////////////////////////////////
	void RegisterParticleSystem(const struct ParticleSystemInfo_s* ParticleSystemInfo);
	void UnregisterParticleSystem(const struct ParticleSystemInfo_s* ParticleSystemInfo);
	const std::vector<const struct ParticleSystemInfo_s*>& GetParticleSystems() const { return ParticleSystems; }

private:
	std::vector<class IRenderable_c*> RenderableComponents;
	bool RenderSceneDirty = false;

	std::vector<const struct ParticleSystemInfo_s*> ParticleSystems;
};