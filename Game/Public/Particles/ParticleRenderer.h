#pragma once

#include "RenderUtils/RenderGraph/RenderGraph.h"

#include <SurfMath.h>

#include <string>

struct ParticleSystemInfo_s
{
	std::string Name;
	float3 Position; // Spawn location for particles
	float Scale = 0.1f; // Size of particle billboard

	uint32_t MaxCount = 1000u;
	float Lifetime = 5.0f; // how long until a particle dies
	float SpawnRate = 100.0f; // how many per second
	float MaxAngle = 5.0f; // Choose between 0 and this in a random cone
	float3 SpawnDirection = float3(0.0f, 1.0f, 0.0f);
	float VelocityMin = 0.1f;
	float VelocityMax = 1.0f; // rand between min and max in units per second
};

struct ParticleRenderer_s
{
	void Init();

	void AddPass(RenderGraphBuilder_s& RGBuilder, const std::vector<const ParticleSystemInfo_s*>& ParticleSystems, RenderGraphResourceHandle_t SceneColor, RenderGraphResourceHandle_t SceneDepth, FrameBufferAlloc_s ViewUniforms, uint2 ScreenSize);

protected:

	std::shared_ptr<class MaterialShaderInstance_c> BillboardParticleMaterial;
};