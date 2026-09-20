#include "BackgroundMatteMaterial.h"

#include <Assets/TextureManager.h>
#include <Rendering/SpaceRenderer.h>
#include <Rendering/Texture.h>
#include <Render/Render.h>
#include <Shared/FileUtils/PathUtils.h>

namespace TextureBindings
{
    enum
    {
        Matte,
        Count
    };
}

BackgroundMatteMaterialShader_c::BackgroundMatteMaterialShader_c()
{
    ASSIGN_SHADER_PARAM(float3, Color);
    ASSIGN_SHADER_PARAM(float, Contrast);
    ASSIGN_SHADER_PARAM(float, Brightness);
    ASSIGN_SHADER_PARAM(TextureIndex, MatteTextureIndex);
}

void BackgroundMatteMaterialShader_c::Load()
{
    BoundTextures.resize(TextureBindings::Count);

    BoundTextures[TextureBindings::Matte] = TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_BackgroundMatte.hp_tex"));
}

bool BackgroundMatteMaterialShader_c::Compile()
{
    rl::GraphicsPipelineStateDesc PSODesc = MakeDefaultPSODesc(Path_s(PathDirectory_e::Shaders, L"Materials/BackgroundMatte.hlsl"), {});

    PSODesc.DebugName = L"BackgroundMatteShader";
    PSO = rl::CreateGraphicsPipelineState(PSODesc);

    PSODesc.Cull = rl::CullMode::FRONT;
    PSODesc.DebugName = L"BackgroundMatteShaderMirrored";
    PSOMirrored = rl::CreateGraphicsPipelineState(PSODesc);

    return PSO.IsValid() && PSOMirrored.IsValid();
}

rl::GraphicsPipelineState_t BackgroundMatteMaterialShader_c::GetPSO(bool Mirrored)
{
    return Mirrored ? PSOMirrored : PSO;
}

void BackgroundMatteMaterialShader_c::GetDefaultParams(std::vector<uint8_t>& OutData) const
{
    OutData.resize(sizeof(Parameters_s));
    Parameters_s* Params = reinterpret_cast<Parameters_s*>(OutData.data());
    Params->Brightness = 1.0f;
    Params->Contrast = 1.0f;
    Params->Color = float3(1.0f);
    Params->MatteTextureIndex = GetTextureBindIndex(TextureBindings::Matte);
}
