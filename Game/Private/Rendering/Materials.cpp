#include "Rendering/Materials.h"

#include "Rendering/SpaceRenderer.h"

#include <Render/Render.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

rl::GraphicsPipelineState_t MaterialShader_c::GetPSO()
{
    return rl::GraphicsPipelineState_t::INVALID;
}

rl::ConstantBuffer_t MaterialShader_c::GetConstantBuffer()
{
    return rl::ConstantBuffer_t::INVALID;
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

DefaultMaterialShader_c::DefaultMaterialShader_c() : MaterialShader_c()
{
    ShaderParameters["Color"] = SHADER_PARAM(float3, Color);
}

bool DefaultMaterialShader_c::Compile()
{
    const std::string ShaderPath = Path_s(PathDirectory_e::Shaders, L"Game", L"BasicMaterial.hlsl").ToString();

    rl::GraphicsPipelineStateDesc PSODesc = {};
    PSODesc.RasterizerDesc(rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::BACK)
        .DepthDesc(true, rl::ComparisionFunc::LESS_EQUAL)
        .TargetBlendDesc({ rl::RenderFormat::R16G16B16A16_FLOAT }, { rl::BlendMode::None() }, rl::RenderFormat::D32_FLOAT)
        .VertexShader(rl::CreateVertexShader(ShaderPath.c_str()))
        .PixelShader(rl::CreatePixelShader(ShaderPath.c_str()))
        .RootSignature(SpaceRenderer_c::GetRootSignature());

    PSODesc.DebugName = L"DefaultMaterialShader";
    PSO = rl::CreateGraphicsPipelineState(PSODesc);

    return PSO.IsValid();
}

rl::GraphicsPipelineState_t DefaultMaterialShader_c::GetPSO()
{
    return PSO;
}

void DefaultMaterialShader_c::GetDefaultParams(std::vector<uint8_t>& OutData) const
{
    OutData.resize(GetShaderParamBufferSize(), 0u);
    Parameters_s* Params = reinterpret_cast<Parameters_s*>(OutData.data());
    Params->Color = float3(0.5f);
}

MaterialShaderInstance_c::~MaterialShaderInstance_c()
{}

void MaterialShaderInstance_c::SetParent(const std::shared_ptr<MaterialShader_c>& InParent)
{
    Parent = InParent;
    ParamData.clear();

    if (Parent)
    {
        Parent->GetDefaultParams(ParamData);
    }
}

void MaterialShaderInstance_c::SetValue(const MaterialShader_c::ShaderParam_s* Param, const void* Data, size_t DataSize)
{
    CHECK(Param->Size == DataSize);
    CHECK(ParamData.size() >= (Param->Offset + Param->Size));
    memcpy(ParamData.data() + Param->Offset, Data, DataSize);
}


void MaterialShaderInstance_c::SetFloat(const std::string& Param, float Value)
{
    if (const MaterialShader_c::ShaderParam_s* Found = FindParam(Param))
    {
        SetValue(Found, &Value, sizeof(Value));
    }
}

void MaterialShaderInstance_c::SetFloat2(const std::string& Param, float2 Value)
{
    if (const MaterialShader_c::ShaderParam_s* Found = FindParam(Param))
    {
        SetValue(Found, &Value, sizeof(Value));
    }
}

void MaterialShaderInstance_c::SetFloat3(const std::string& Param, float3 Value)
{
    if (const MaterialShader_c::ShaderParam_s* Found = FindParam(Param))
    {
        SetValue(Found, &Value, sizeof(Value));
    }
}

void MaterialShaderInstance_c::SetFloat4(const std::string& Param, float4 Value)
{
    if (const MaterialShader_c::ShaderParam_s* Found = FindParam(Param))
    {
        SetValue(Found, &Value, sizeof(Value));
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

void MaterialShaderInstance_c::Update()
{
    if (ENSUREMSG(!ConstantBuffer.IsValid(), "[MaterialShaderInstance_c::Update] Cannot update already built instance"))
    {
        ConstantBuffer = rl::CreateConstantBuffer(ParamData.data(), ParamData.size());
    }   
}

rl::GraphicsPipelineState_t MaterialShaderInstance_c::GetPSO()
{
    return Parent != nullptr ? Parent->GetPSO() : rl::GraphicsPipelineState_t::INVALID;
}

rl::ConstantBuffer_t MaterialShaderInstance_c::GetConstantBuffer()
{
    return ConstantBuffer;
}
