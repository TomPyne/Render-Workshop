#include "BRDFMaterial.h"

#include "Rendering/SpaceRenderer.h"

#include <Render/Render.h>
#include <Shared/FileUtils/PathUtils.h>

BRDFMaterialShader_c::BRDFMaterialShader_c()
{
    ASSIGN_SHADER_PARAM(float3, Albedo);
}

bool BRDFMaterialShader_c::Compile()
{
    const std::string ShaderPath = Path_s(PathDirectory_e::Shaders, L"Game", L"Materials/DefaultBRDF.hlsl").ToString();

    rl::GraphicsPipelineStateDesc PSODesc = {};
    PSODesc.RasterizerDesc(rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::BACK)
        .DepthDesc(true, rl::ComparisionFunc::LESS_EQUAL)
        .TargetBlendDesc(SpaceRenderer_c::GetMaterialPipelineTargetDesc())
        .VertexShader(rl::CreateVertexShader(ShaderPath.c_str()))
        .PixelShader(rl::CreatePixelShader(ShaderPath.c_str()))
        .RootSignature(SpaceRenderer_c::GetRootSignature());

    PSODesc.DebugName = L"DefaultBRDFShader";
    PSO = rl::CreateGraphicsPipelineState(PSODesc);

    // A mirroring transform reverses triangle winding, so those draws need the opposite cull face.
    PSODesc.Cull = rl::CullMode::FRONT;
    PSODesc.DebugName = L"DefaultBRDFShaderMirrored";
    PSOMirrored = rl::CreateGraphicsPipelineState(PSODesc);

    return PSO.IsValid() && PSOMirrored.IsValid();
}

rl::GraphicsPipelineState_t BRDFMaterialShader_c::GetPSO(bool Mirrored)
{
    return Mirrored ? PSOMirrored : PSO;
}

void BRDFMaterialShader_c::GetDefaultParams(std::vector<uint8_t>& OutData) const
{
    OutData.resize(sizeof(Parameters_s));
    Parameters_s* Params = reinterpret_cast<Parameters_s*>(OutData.data());
    Params->Albedo = float3(1.0f);
}
