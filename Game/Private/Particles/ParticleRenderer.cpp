#include "Particles/ParticleRenderer.h"

#include "Assets/MaterialManager.h"
#include "Rendering/Materials.h"
#include "Rendering/SpaceRenderer.h"

#include <RenderUtils/GPUContext/GPUContext.h>
#include <Shared/Logging/Logging.h>

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
			{ rl::RenderFormat::R16G16B16A16_FLOAT },
			{ rl::BlendMode::Default()},
            SpaceRenderer_c::GetMaterialPipelineDepthFormat());

        rl::GraphicsPipelineStateDesc PSODesc = {};
        PSODesc.RasterizerDesc(rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::BACK)
            .DepthDesc(true, rl::ComparisionFunc::LESS_EQUAL, rl::DepthWriteMask::ZERO)
            .TargetBlendDesc(MaterialPipelineTargetDesc)
            .VertexShader(rl::CreateVertexShader(ShaderPath.c_str()))
            .PixelShader(rl::CreatePixelShader(ShaderPath.c_str()))
            .RootSignature(SpaceRenderer_c::GetRootSignature());

        PSODesc;

        PSODesc.DebugName = ShaderDebugName;
        PSO = rl::CreateGraphicsPipelineState(PSODesc);

        return PSO.IsValid() && PSOMirrored.IsValid();
    }

    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override { InitDefaultParams<Parameters_s>(OutData); }
};

void ParticleRenderer_s::Init()
{
    MaterialManager::RegisterMaterialShaderClass<ParticleBillboardMaterialShader_c>(L"ParticleShader");
    BillboardParticleMaterial = MaterialManager::RequestMaterialInstance(Path_s(PathDirectory_e::Assets, L"Game", L"Materials/Particles/DefaultSprite.hp_mtl"));
}


void ParticleRenderer_s::AddPass(RenderGraphBuilder_s& RGBuilder, const std::vector<const ParticleSystemInfo_s*>& ParticleSystems, RenderGraphResourceHandle_t SceneColor, RenderGraphResourceHandle_t SceneDepth, FrameBufferAlloc_s ViewUniforms, uint2 ScreenSize)
{
	if (ParticleSystems.empty() || !BillboardParticleMaterial)
		return;

    RenderGraphPass_s& MeshDrawPass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Particle Pass")
    .AccessResource(SceneColor, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::LOAD)
    .AccessResource(SceneDepth, RenderGraphResourceAccessType_e::DSV, RenderGraphLoadOp_e::LOAD) // Reads depth only, maybe hinting this to RG is an optimization
    .SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
    {

        // 1. Run a pass that handles lifetime and spawn rate.
		// We should use a ring buffer sized to the max number of particles, and spawn new particles into the buffer as needed.
		// If we spawn particles at a rate that exceeds the max number of particles, we should overwrite the oldest particles in the buffer.

        // 2. Run a pass that updates the position of the particles.
        // It should use projectile motion with gravity, so the initial velocity bleeds off and it falls back to the ground.

		// 3. We need to generate the indirect draw args using the particle count and the number of particles in the ring buffer. 
        // This should be done in a compute shader that writes to an indirect draw args buffer.

         // 4. Modify the drawing code below to use an indirect draw, ParticleUniforms_s becomes defunct, the shader reads into the particle buffer for position. 

        Ctx.SetRootSignature(SpaceRenderer_c::GetRootSignature());

        rl::RenderTargetView_t SceneRTVs[] = { RG.GetRTV(SceneColor) };
        Ctx.SetRenderTargets(SceneRTVs, ARRAYSIZE(SceneRTVs), RG.GetDSV(SceneDepth));

        rl::Viewport vp{ ScreenSize.x, ScreenSize.y };
        Ctx.SetViewports(&vp, 1);
        Ctx.SetDefaultScissor(); // Could also be captured by the command context

        Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_VIEW_BUF, ViewUniforms);
        Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_MAT_BUF, BillboardParticleMaterial->GetConstantBuffer());
        Ctx.SetGraphicsRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE);

        struct ParticleUniforms_s
        {
            float3 Position;
            float Size;
        } Uniforms;
        Uniforms.Size = 1.0f; // Default size, can be modified based on particle system properties

        for (const ParticleSystemInfo_s* ParticleSystem : ParticleSystems)
        {
            if (!ParticleSystem)
                continue;

            Uniforms.Position = ParticleSystem->Position;

            Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_MODEL_BUF, RG.Alloc(Uniforms));

            Ctx.SetPipelineState(BillboardParticleMaterial->GetPSO(Uniforms.Size < 0.0f));
            Ctx.DrawInstanced(6u, 1u, 0u, 0u);
        }
    });
}
