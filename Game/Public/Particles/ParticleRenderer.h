#pragma once

#include "RenderUtils/RenderGraph/RenderGraph.h"

#include <Render/IndirectCommands.h>
#include <SurfMath.h>

#include <memory>
#include <string>

struct ParticleSystemRenderData_s;

struct ParticleSystemInfo_s
{
	// Defined next to ParticleSystemRenderData_s so unique_ptr can delete it, the constructor needs it for exception cleanup
	ParticleSystemInfo_s();
	~ParticleSystemInfo_s();

	std::string Name;
	float3 Position; // Spawn location for particles
	float Scale = 0.1f; // Size of particle billboard

	uint32_t MaxCount = 1000u;
	float Lifetime = 5.0f; // how long until a particle dies
	float SpawnRate = 100.0f; // how many per second
	float MaxAngle = 20.0f; // Choose between 0 and this in a random cone
	float3 SpawnDirection = float3(0.0f, 1.0f, 0.0f);
	float VelocityMin = 8.0f;
	float VelocityMax = 16.0f; // rand between min and max in units per second

	mutable std::unique_ptr<ParticleSystemRenderData_s> RenderData; // Owned by the renderer, created lazily
};

struct ParticleRenderer_s
{
	void Init();

	void AddPass(RenderGraphBuilder_s& RGBuilder, const std::vector<const ParticleSystemInfo_s*>& ParticleSystems, RenderGraphResourceHandle_t SceneColor, RenderGraphResourceHandle_t SceneDepth, FrameBufferAlloc_s ViewUniforms, uint2 ScreenSize, float DeltaSeconds, uint64_t FrameIndex);

protected:

	void CreateRenderData(const ParticleSystemInfo_s& ParticleSystem);

	std::shared_ptr<class MaterialShaderInstance_c> BillboardParticleMaterial;

	rl::ComputePipelineStatePtr ResetArgsPSO;
	rl::ComputePipelineStatePtr SimulatePSO;
	rl::RenderPtr<rl::IndirectCommand_t> DrawCommand;

	uint32_t NextSeed = 0u;
};