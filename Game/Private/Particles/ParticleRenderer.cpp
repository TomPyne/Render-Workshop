#include "Particles/ParticleRenderer.h"

#include "Assets/MaterialManager.h"
#include "Rendering/GlobalDistanceField.h"
#include "Rendering/Materials.h"
#include "Rendering/SpaceRenderer.h"

#include <Render/Render.h>
#include <RenderUtils/GPUContext/GPUContext.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

// Must match Particle_s in Particles/ParticleCommon.h
struct Particle_s
{
	float3 Position;
	float Age;

	float3 Velocity;
	float Lifetime;
};
static_assert(sizeof(Particle_s) == 32);

// Must match ParticleDrawArgs_s in Particles/ParticleCommon.h
struct ParticleDrawArgs_s
{
	uint32_t VertexCountPerInstance;
	uint32_t InstanceCount;
	uint32_t StartVertexLocation;
	uint32_t StartInstanceLocation;
};

// Must match ParticleSystemUniforms_s in Particles/ParticleCommon.h
struct ParticleSystemUniforms_s
{
	float3 EmitterPosition;
	float DeltaTime;

	float3 SpawnDirection;
	float MaxAngleRadians;

	float VelocityMin;
	float VelocityMax;
	float Lifetime;
	float Scale;

	uint32_t MaxCount;
	uint32_t SpawnStart;
	uint32_t SpawnCount;
	uint32_t Seed;

	uint32_t FrameIndex;
	uint32_t ParticlesUAV;
	uint32_t AliveListUAV;
	uint32_t DrawArgsUAV;

	uint32_t ParticlesSRV;
	uint32_t AliveListSRV;
	ParticleCollisionMode_e CollisionMode;
	uint32_t __Pad;

	float CollisionRadius;
	float Restitution;
	float Friction;
	float RestSpeed;
};
static_assert(sizeof(ParticleSystemUniforms_s) == 112);

// Must match ParticleVolumeUniforms_s in Particles/ParticleSimulate.hlsl
struct ParticleVolumeUniforms_s
{
	float3 VolumeMin;
	float VoxelSize;

	float3 VolumeMax;
	uint32_t VolumeSRV;

	uint32_t Enabled;
	uint32_t __Pad[3];
};
static_assert(sizeof(ParticleVolumeUniforms_s) == 48);

// Large steps tunnel through thin geometry in the distance field, so hitches slow the simulation down instead
static constexpr float MaxParticleDeltaSeconds = 1.0f / 30.0f;
static constexpr uint32_t ParticleSimulateGroupSize = 64u; // Must match numthreads in ParticleSimulate.hlsl

struct ParticleSystemRenderData_s
{
	rl::StructuredBufferPtr Particles;
	rl::StructuredBufferPtr AliveList;
	rl::StructuredBufferPtr DrawArgs;

	rl::UnorderedAccessViewPtr ParticlesUAV;
	rl::ShaderResourceViewPtr ParticlesSRV;
	rl::UnorderedAccessViewPtr AliveListUAV;
	rl::ShaderResourceViewPtr AliveListSRV;
	rl::UnorderedAccessViewPtr DrawArgsUAV;

	uint32_t RingHead = 0u;
	float SpawnAccumulator = 0.0f;
	uint32_t Seed = 0u;
};

ParticleSystemInfo_s::ParticleSystemInfo_s() = default;
ParticleSystemInfo_s::~ParticleSystemInfo_s() = default;

class ParticleBillboardMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float3 Color = float3(1.0f, 1.0f, 1.0f);
        TextureIndex AlbedoAlphaTexture = 0u;
    };

public:

    ParticleBillboardMaterialShader_c()
    {
        ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Game", L"Particles/ParticleSprite.hlsl");
        ShaderDebugName = L"ParticleBillboardMaterialShader";

        ASSIGN_SHADER_PARAM(float3, Color);
        ASSIGN_SHADER_PARAM(TextureIndex, AlbedoAlphaTexture);
    }

    virtual void Load() override
    {
		LoadShaderTexture("AlbedoAlphaTexture", Path_s(PathDirectory_e::Assets, L"Game", L"Textures/ParticleSprite.hp_tex"));
    }

    virtual bool Compile() override
    {
        if (!ShaderFilePath.IsValid())
        {
            LOGWARNING("[MaterialShader_c::Compile] Failed to compile due to ShaderFilePath being empty");
            return false;
        }

        const std::string ShaderPath = ShaderFilePath.ToString();

        static rl::GraphicsPipelineTargetDesc MaterialPipelineTargetDesc = rl::GraphicsPipelineTargetDesc(
			{ rl::RenderFormat::R11G11B10_FLOAT },
			{ rl::BlendMode::Add() },
            SpaceRenderer_c::GetMaterialPipelineDepthFormat());

        rl::GraphicsPipelineStateDesc PSODesc = {};
        PSODesc.RasterizerDesc(rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::BACK)
            .DepthDesc(true, rl::ComparisionFunc::LESS_EQUAL, rl::DepthWriteMask::ZERO)
            .TargetBlendDesc(MaterialPipelineTargetDesc)
            .VertexShader(rl::CreateVertexShader(ShaderPath.c_str()))
            .PixelShader(rl::CreatePixelShader(ShaderPath.c_str()))
            .RootSignature(SpaceRenderer_c::GetRootSignature());

        PSODesc.DebugName = ShaderDebugName;
        PSO = rl::CreateGraphicsPipelineState(PSODesc);

        return PSO.IsValid();
    }

    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override { InitDefaultParams<Parameters_s>(OutData); }
};

void ParticleRenderer_s::Init()
{
    MaterialManager::RegisterMaterialShaderClass<ParticleBillboardMaterialShader_c>(L"ParticleShader");
    BillboardParticleMaterial = MaterialManager::RequestMaterialInstance(Path_s(PathDirectory_e::Assets, L"Game", L"Materials/Particles/DefaultSprite.hp_mtl"));

	static const Path_s SimulatePath = Path_s(PathDirectory_e::Shaders, L"Game", L"Particles/ParticleSimulate.hlsl");

	rl::ComputePipelineStateDesc PSODesc = {};
	PSODesc.RootSignatureOverride = SpaceRenderer_c::GetRootSignature();

	PSODesc.Cs = rl::CreateComputeShader(SimulatePath.ToString().c_str(), { { "PARTICLE_RESET_ARGS" } });
	PSODesc.DebugName = L"ParticleResetArgsCS";
	ResetArgsPSO = rl::CreateComputePipelineState(PSODesc);

	PSODesc.Cs = rl::CreateComputeShader(SimulatePath.ToString().c_str(), { { "PARTICLE_SIMULATE" } });
	PSODesc.DebugName = L"ParticleSimulateCS";
	SimulatePSO = rl::CreateComputePipelineState(PSODesc);

	DrawCommand = rl::CreateIndirectDrawCommand();
}

void ParticleRenderer_s::CreateRenderData(const ParticleSystemInfo_s& ParticleSystem)
{
	const uint32_t MaxCount = ParticleSystem.MaxCount;

	std::unique_ptr<ParticleSystemRenderData_s> Data = std::make_unique<ParticleSystemRenderData_s>();

	// Zeroed particles have Age == Lifetime, so they start dead
	const std::vector<Particle_s> InitialParticles(MaxCount, Particle_s{});
	const std::vector<uint32_t> InitialAliveList(MaxCount, 0u);
	const ParticleDrawArgs_s InitialDrawArgs = { 6u, 0u, 0u, 0u };

	Data->Particles = rl::CreateRWStructuredBuffer(InitialParticles.data(), InitialParticles.size());
	Data->AliveList = rl::CreateRWStructuredBuffer(InitialAliveList.data(), InitialAliveList.size());
	Data->DrawArgs = rl::CreateRWStructuredBuffer(&InitialDrawArgs, 1u);

	Data->ParticlesUAV = rl::CreateStructuredBufferUAV(Data->Particles, 0u, MaxCount, sizeof(Particle_s));
	Data->ParticlesSRV = rl::CreateStructuredBufferSRV(Data->Particles, 0u, MaxCount, sizeof(Particle_s));
	Data->AliveListUAV = rl::CreateStructuredBufferUAV(Data->AliveList, 0u, MaxCount, sizeof(uint32_t));
	Data->AliveListSRV = rl::CreateStructuredBufferSRV(Data->AliveList, 0u, MaxCount, sizeof(uint32_t));
	Data->DrawArgsUAV = rl::CreateStructuredBufferUAV(Data->DrawArgs, 0u, 1u, sizeof(ParticleDrawArgs_s));

	Data->Seed = NextSeed++;

	ParticleSystem.RenderData = std::move(Data);
}


void ParticleRenderer_s::AddPass(RenderGraphBuilder_s& RGBuilder, const std::vector<const ParticleSystemInfo_s*>& ParticleSystems, RenderGraphResourceHandle_t SceneColor, RenderGraphResourceHandle_t SceneDepth, FrameBufferAlloc_s ViewUniforms, uint2 ScreenSize, float DeltaSeconds, uint64_t FrameIndex,
	const GlobalDistanceField_c& GlobalDistanceField, RenderGraphResourceHandle_t GlobalVolume)
{
	if (ParticleSystems.empty() || !BillboardParticleMaterial || !ResetArgsPSO.IsValid() || !SimulatePSO.IsValid())
		return;

	struct ParticleSystemDraw_s
	{
		rl::StructuredBuffer_t Particles;
		rl::StructuredBuffer_t AliveList;
		rl::StructuredBuffer_t DrawArgs;
		uint32_t MaxCount;
		FrameBufferAlloc_s Uniforms;
	};

	std::vector<ParticleSystemDraw_s> Draws;
	Draws.reserve(ParticleSystems.size());

	const float DeltaTime = Min(DeltaSeconds, MaxParticleDeltaSeconds);
	const bool Simulate = DeltaTime > 0.0f;

	for (const ParticleSystemInfo_s* ParticleSystem : ParticleSystems)
	{
		if (!ParticleSystem || ParticleSystem->MaxCount == 0u)
			continue;

		if (!ParticleSystem->RenderData)
		{
			// Initial contents upload at the start of next frame, until then the buffers hold garbage and aren't in READ
			CreateRenderData(*ParticleSystem);
			continue;
		}

		ParticleSystemRenderData_s& Data = *ParticleSystem->RenderData;

		const uint32_t MaxCount = ParticleSystem->MaxCount;

		uint32_t SpawnCount = 0u;
		const uint32_t SpawnStart = Data.RingHead;
		
		if (Simulate)
		{
			// Spawn rate is floating point, so rather than rounding down accumulate every frame until we have a round number. Means
			// that spawn rates are never biased down by rounding, and means rates less than 1.0 can actually spawn
			Data.SpawnAccumulator += ParticleSystem->SpawnRate * DeltaTime;
			const float Spawned = std::floor(Data.SpawnAccumulator);
			Data.SpawnAccumulator -= Spawned;

			SpawnCount = Min(static_cast<uint32_t>(Spawned), MaxCount);
			Data.RingHead = (Data.RingHead + SpawnCount) % MaxCount;
		}

		ParticleSystemUniforms_s Uniforms = {};
		Uniforms.EmitterPosition = ParticleSystem->Position;
		Uniforms.DeltaTime = DeltaTime;
		Uniforms.SpawnDirection = ParticleSystem->SpawnDirection;
		Uniforms.MaxAngleRadians = ConvertToRadians(ParticleSystem->MaxAngle);
		Uniforms.VelocityMin = ParticleSystem->VelocityMin;
		Uniforms.VelocityMax = ParticleSystem->VelocityMax;
		Uniforms.Lifetime = ParticleSystem->Lifetime;
		Uniforms.Scale = ParticleSystem->Scale;
		Uniforms.MaxCount = MaxCount;
		Uniforms.SpawnStart = SpawnStart;
		Uniforms.SpawnCount = SpawnCount;
		Uniforms.Seed = Data.Seed;
		Uniforms.FrameIndex = static_cast<uint32_t>(FrameIndex);
		Uniforms.ParticlesUAV = rl::GetDescriptorIndex(Data.ParticlesUAV);
		Uniforms.AliveListUAV = rl::GetDescriptorIndex(Data.AliveListUAV);
		Uniforms.DrawArgsUAV = rl::GetDescriptorIndex(Data.DrawArgsUAV);
		Uniforms.ParticlesSRV = rl::GetDescriptorIndex(Data.ParticlesSRV);
		Uniforms.AliveListSRV = rl::GetDescriptorIndex(Data.AliveListSRV);
		Uniforms.CollisionMode = ParticleSystem->CollisionMode;
		Uniforms.CollisionRadius = ParticleSystem->CollisionRadius;
		Uniforms.Restitution = ParticleSystem->Restitution;
		Uniforms.Friction = ParticleSystem->Friction;
		Uniforms.RestSpeed = ParticleSystem->RestSpeed;

		Draws.push_back({ Data.Particles, Data.AliveList, Data.DrawArgs, MaxCount, RGBuilder.Alloc(Uniforms) });
	}

	if (Draws.empty())
		return;

	const bool CollisionEnabled = Simulate && GlobalVolume != RenderGraphResourceHandle_t::NONE;
	const AABB VolumeBounds = GlobalDistanceField.GetVolumeBounds();
	const float VoxelSize = GlobalDistanceField.GetVoxelSize();

	RenderGraphPass_s& Pass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Particle Pass")
	.AccessResource(SceneColor, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneDepth, RenderGraphResourceAccessType_e::DSV, RenderGraphLoadOp_e::LOAD); // Reads depth only, maybe hinting this to RG is an optimization

	if (CollisionEnabled)
	{
		Pass.AccessResource(GlobalVolume, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD);
	}

	Pass.SetExecuteCallback([this, Draws, Simulate, CollisionEnabled, VolumeBounds, VoxelSize, GlobalVolume, SceneColor, SceneDepth, ViewUniforms, ScreenSize](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		Ctx.SetRootSignature(SpaceRenderer_c::GetRootSignature());

		// The particle buffers are invisible to the RG, so each one leaves this pass in READ as it entered
		if (Simulate)
		{
			Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_UAV_TABLE);
			Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE);

			ParticleVolumeUniforms_s VolumeUniforms = {};
			VolumeUniforms.VolumeMin = VolumeBounds.mins;
			VolumeUniforms.VoxelSize = VoxelSize;
			VolumeUniforms.VolumeMax = VolumeBounds.maxs;
			VolumeUniforms.VolumeSRV = CollisionEnabled ? RG.GetSRVIndex(GlobalVolume) : 0u;
			VolumeUniforms.Enabled = CollisionEnabled ? 1u : 0u;
			Ctx.SetComputeRootCBV(SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, RG.Alloc(VolumeUniforms));

			for (const ParticleSystemDraw_s& Draw : Draws)
			{
				Ctx.TransitionResource(Draw.Particles, rl::ResourceTransitionState::READ, rl::ResourceTransitionState::UNORDERED_ACCESS);
				Ctx.TransitionResource(Draw.AliveList, rl::ResourceTransitionState::READ, rl::ResourceTransitionState::UNORDERED_ACCESS);
				Ctx.TransitionResource(Draw.DrawArgs, rl::ResourceTransitionState::READ, rl::ResourceTransitionState::UNORDERED_ACCESS);

				Ctx.SetComputeRootCBV(SpaceRendererRootSigSlots::RS_MODEL_BUF, Draw.Uniforms);

				Ctx.SetPipelineState(ResetArgsPSO.Get());
				Ctx.Dispatch(1u, 1u, 1u);

				Ctx.RWBarrier(Draw.DrawArgs);

				Ctx.SetPipelineState(SimulatePSO.Get());
				Ctx.Dispatch(DivideRoundUp(Draw.MaxCount, ParticleSimulateGroupSize), 1u, 1u);

				Ctx.TransitionResource(Draw.Particles, rl::ResourceTransitionState::UNORDERED_ACCESS, rl::ResourceTransitionState::READ);
				Ctx.TransitionResource(Draw.AliveList, rl::ResourceTransitionState::UNORDERED_ACCESS, rl::ResourceTransitionState::READ);
				Ctx.TransitionResource(Draw.DrawArgs, rl::ResourceTransitionState::UNORDERED_ACCESS, rl::ResourceTransitionState::READ);
			}
		}

		rl::RenderTargetView_t SceneRTVs[] = { RG.GetRTV(SceneColor) };
		Ctx.SetRenderTargets(SceneRTVs, ARRAYSIZE(SceneRTVs), RG.GetDSV(SceneDepth));

		rl::Viewport vp{ ScreenSize.x, ScreenSize.y };
		Ctx.SetViewports(&vp, 1);
		Ctx.SetDefaultScissor(); // Could also be captured by the command context

		Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_VIEW_BUF, ViewUniforms);
		Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_MAT_BUF, BillboardParticleMaterial->GetConstantBuffer());
		Ctx.SetGraphicsRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE);
		Ctx.SetPipelineState(BillboardParticleMaterial->GetPSO(false));

		for (const ParticleSystemDraw_s& Draw : Draws)
		{
			Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_MODEL_BUF, Draw.Uniforms);
			Ctx.ExecuteIndirect(DrawCommand.Get(), Draw.DrawArgs);
		}
	});
}
