#include "ArchMaterial.h"

#include <Assets/TextureManager.h>
#include <Rendering/SpaceRenderer.h>
#include <Rendering/Texture.h>
#include <Render/Render.h>
#include <Shared/FileUtils/PathUtils.h>

namespace TextureBindings
{
    enum
    {
        Albedo,
        Normal,
        DetailNormal,
        Mask,
        Count
    };
}

ArchMaterialShader_c::ArchMaterialShader_c()
{
    ASSIGN_SHADER_PARAM(float3, ColorMarble1);
    ASSIGN_SHADER_PARAM(float, AOStrength);
    ASSIGN_SHADER_PARAM(float3, ColorMarble2);
    ASSIGN_SHADER_PARAM(float, AOStrength2);
    ASSIGN_SHADER_PARAM(float, NormalIntensity);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble1);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble2);
    ASSIGN_SHADER_PARAM(float, ScaleMarble1);
    ASSIGN_SHADER_PARAM(float, ScaleMarble2);
    ASSIGN_SHADER_PARAM(TextureIndex, MaskTextureIndex);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTextureIndex);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTextureIndex);
    ASSIGN_SHADER_PARAM(TextureIndex, DetailNormalTextureIndex);
}

void ArchMaterialShader_c::Load()
{
    BoundTextures.resize(TextureBindings::Count);

    BoundTextures[TextureBindings::Albedo] = TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FloorMarble_D.hp_tex"));
    BoundTextures[TextureBindings::Normal] = TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Arch_N.hp_tex"));
    BoundTextures[TextureBindings::DetailNormal] = TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Marble_N.hp_tex"));
    BoundTextures[TextureBindings::Mask] = TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Arch_M.hp_tex"));
}

bool ArchMaterialShader_c::Compile()
{
    const std::string ShaderPath = Path_s(PathDirectory_e::Shaders, L"Materials/Arch.hlsl").ToString();

    rl::ShaderMacros Macros = { rl::ShaderMacro("USE_UV3", "0")};

    rl::GraphicsPipelineStateDesc PSODesc = {};
    PSODesc.RasterizerDesc(rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::BACK)
        .DepthDesc(true, rl::ComparisionFunc::LESS_EQUAL)
        .TargetBlendDesc(SpaceRenderer_c::GetMaterialPipelineTargetDesc())
        .VertexShader(rl::CreateVertexShader(ShaderPath.c_str(), Macros))
        .PixelShader(rl::CreatePixelShader(ShaderPath.c_str(), Macros))
        .RootSignature(SpaceRenderer_c::GetRootSignature());

    PSODesc.DebugName = L"ArchShader";
    PSO = rl::CreateGraphicsPipelineState(PSODesc);

    PSODesc.Cull = rl::CullMode::FRONT;
    PSODesc.DebugName = L"ArchShaderMirrored";
    PSOMirrored = rl::CreateGraphicsPipelineState(PSODesc);

    return PSO.IsValid() && PSOMirrored.IsValid();
}

rl::GraphicsPipelineState_t ArchMaterialShader_c::GetPSO(bool Mirrored)
{
    return Mirrored ? PSOMirrored : PSO;
}

void ArchMaterialShader_c::GetDefaultParams(std::vector<uint8_t>& OutData) const
{
    OutData.resize(sizeof(Parameters_s));
    Parameters_s* Params = reinterpret_cast<Parameters_s*>(OutData.data());
    Params->AOStrength2 = 0.0f;
    Params->AOStrength = 0.0f;
    Params->ColorMarble1 = float3(1.0f, 1.0f, 1.0f);
    Params->ColorMarble2 = float3(1.0f, 1.0f, 1.0f);
    Params->NormalIntensity = 0.5f;
    Params->RoughnessMarble1 = 0.3f;
    Params->RoughnessMarble2 = 0.8f;
    Params->ScaleMarble1 = 1.0f;
    Params->ScaleMarble2 = 1.0f;
    Params->AlbedoTextureIndex = GetTextureBindIndex(TextureBindings::Albedo);
    Params->NormalTextureIndex = GetTextureBindIndex(TextureBindings::Normal);
    Params->DetailNormalTextureIndex = GetTextureBindIndex(TextureBindings::DetailNormal);
    Params->MaskTextureIndex = GetTextureBindIndex(TextureBindings::Mask);
}
