#include "Rendering/Materials.h"

#include "Rendering/SpaceRenderer.h"

#include <Render/Render.h>
#include <Shared/Logging/Logging.h>

// ## BasicShader
// BasicShader contains the shaders, PSOs, and buffers

BasicMaterial_c* MakeBasicMaterial(float3 Color)
{
    BasicMaterial_c* NewMaterial = new BasicMaterial_c;
    static const char* ShaderPath = "Game/Shaders/BasicMaterial.hlsl";
    rl::VertexShader_t MeshVS = rl::CreateVertexShader(ShaderPath);
    rl::PixelShader_t MeshPS = rl::CreatePixelShader(ShaderPath);

    rl::GraphicsPipelineStateDesc PsoDesc = {};
    PsoDesc.RasterizerDesc(rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::NONE)
        .DepthDesc(true, rl::ComparisionFunc::LESS_EQUAL)
        .TargetBlendDesc({ rl::RenderFormat::R16G16B16A16_FLOAT, rl::RenderFormat::R16G16B16A16_FLOAT, rl::RenderFormat::R16G16_FLOAT, rl::RenderFormat::R16G16_FLOAT }, { rl::BlendMode::None(), rl::BlendMode::None(), rl::BlendMode::None(), rl::BlendMode::None() }, rl::RenderFormat::D32_FLOAT)
        .VertexShader(MeshVS)
        .PixelShader(MeshPS)
        .RootSignature(SpaceRenderer_c::GetRootSignature());

    PsoDesc.DebugName = L"BasicMaterialPSO";
    NewMaterial->PSO = CreateGraphicsPipelineState(PsoDesc);

	float4 MaterialColor = float4(Color, 1.0f);
    NewMaterial->MaterialConstants = rl::CreateConstantBuffer(&MaterialColor);

    return NewMaterial;
}

void DestroyBasicMaterial(BasicMaterial_c* Material)
{
    if (Material)
        delete Material;
}

rl::GraphicsPipelineState_t MaterialShader_c::GetPSO()
{
    return rl::GraphicsPipelineState_t::INVALID;
}

uint32_t MaterialShader_c::GetShaderParamBufferSize() const
{
    return GetShaderParamBufferSizeFloats() * 4u;
}

const MaterialShader_c::ShaderParam_s* MaterialShader_c::GetShaderParam(const std::string& Param) const
{
    auto FoundIt = ShaderParameters.find(Param);
    return FoundIt != ShaderParameters.end() ? &FoundIt->second : nullptr;
}

void DefaultMaterialShader_c::Init()
{
    ShaderParameters["Color"] = SHADER_PARAM(float3, Color);
}

rl::GraphicsPipelineState_t DefaultMaterialShader_c::GetPSO()
{
    return rl::GraphicsPipelineState_t();
}

void MaterialShaderInstance_c::SetParent(const std::shared_ptr<MaterialShader_c>& InParent)
{
    Parent = InParent;
    ParamData.clear();

    if (Parent)
    {
        ParamData.resize(Parent->GetShaderParamBufferSize());
    }
}

void MaterialShaderInstance_c::SetFloat(const std::string& Param, float Value)
{
    if (const MaterialShader_c::ShaderParam_s* Found = FindParam(Param))
    {
        CHECK(Found->Size > sizeof(Value));
        CHECK(ParamData.size() < (Found->Offset + Found->Size));
        memcpy(ParamData.data() + Found->Offset, &Value, sizeof(Value));
    }
}

void MaterialShaderInstance_c::SetFloat2(const std::string& Param, float2 Value)
{
    if (const MaterialShader_c::ShaderParam_s* Found = FindParam(Param))
    {
        CHECK(Found->Size > sizeof(Value));
        CHECK(ParamData.size() < (Found->Offset + Found->Size));
        memcpy(ParamData.data() + Found->Offset, &Value, sizeof(Value));
    }
}

void MaterialShaderInstance_c::SetFloat3(const std::string& Param, float3 Value)
{
    if (const MaterialShader_c::ShaderParam_s* Found = FindParam(Param))
    {
        CHECK(Found->Size > sizeof(Value));
        CHECK(ParamData.size() < (Found->Offset + Found->Size));
        memcpy(ParamData.data() + Found->Offset, &Value, sizeof(Value));
    }
}

void MaterialShaderInstance_c::SetFloat4(const std::string& Param, float4 Value)
{
    if (const MaterialShader_c::ShaderParam_s* Found = FindParam(Param))
    {
        CHECK(Found->Size > sizeof(Value));
        CHECK(ParamData.size() < (Found->Offset + Found->Size));
        memcpy(ParamData.data() + Found->Offset, &Value, sizeof(Value));
    }
}

const MaterialShader_c::ShaderParam_s* MaterialShaderInstance_c::FindParam(const std::string& Param) const
{
    if (Parent)
    {
        return Parent->GetShaderParam(Param);
    }
    return nullptr;
}
