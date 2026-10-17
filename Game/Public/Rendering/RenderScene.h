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

	// Skylight ///////////////////////////////////////////////////////////////////////////////////
	// Only one skylight is supported, returns false if another is already registered
	bool RegisterSkylight(const struct SkylightInfo_s* SkylightInfo);
	void UnregisterSkylight(const struct SkylightInfo_s* SkylightInfo);
	const struct SkylightInfo_s* GetSkylight() const { return Skylight; }

	// Directional light /////////////////////////////////////////////////////////////////////////
	// Only one directional light is supported, returns false if another is already registered
	bool RegisterDirectionalLight(struct DirectionalLightInfo_s* DirectionalLightInfo);
	void UnregisterDirectionalLight(const struct DirectionalLightInfo_s* DirectionalLightInfo);
	// Mutable so tools can tweak the registered light
	struct DirectionalLightInfo_s* GetDirectionalLight() const { return DirectionalLight; }

private:
	std::vector<class IRenderable_c*> RenderableComponents;
	std::vector<const struct ParticleSystemInfo_s*> ParticleSystems;
	const struct SkylightInfo_s* Skylight = nullptr;
	struct DirectionalLightInfo_s* DirectionalLight = nullptr;
};