#pragma once

#include "RenderUtils/RenderGraph/RenderGraph.h"

#include <SurfMath.h>

#include <string>

struct ParticleSystemInfo_s
{
	std::string Name;
	float3 Position;
	float Scale = 0.1f;

	uint32_t MaxCount = 1000u;
	float Lifetime = 5.0f;
	float SpawnRate = 100.0f;
	float MaxAngle = 5.0f;
	float3 SpawnDirection = float3(0.0f, 1.0f, 0.0f);
	float VelocityMin = 0.1f;
	float VelocityMax = 1.0f;
};

struct ParticleRenderer_s
{
	void Init();

	void AddPass(RenderGraphBuilder_s& RGBuilder, const std::vector<const ParticleSystemInfo_s*>& ParticleSystems, RenderGraphResourceHandle_t SceneColor, RenderGraphResourceHandle_t SceneDepth, FrameBufferAlloc_s ViewUniforms, uint2 ScreenSize);

protected:

	std::shared_ptr<class MaterialShaderInstance_c> BillboardParticleMaterial;
};