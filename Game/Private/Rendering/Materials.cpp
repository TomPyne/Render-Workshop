#include "Rendering/Materials.h"

#include "Rendering/SpaceRenderer.h"
#include "Rendering/Texture.h"

#include <Render/Render.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

rl::GraphicsPipelineState_t MaterialShader_c::GetPSO(bool Mirrored)
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

const ShaderParam_s* MaterialShader_c::FindParam(std::string_view Param) const
{
    auto FoundIt = ShaderParameters.find(std::string(Param)); // Gross construction of string to make this work. TODO - update when using hashed strings
    return FoundIt != ShaderParameters.end() ? &FoundIt->second : nullptr;
}

rl::GraphicsPipelineStateDesc MaterialShader_c::MakeDefaultPSODesc(const Path_s& Path, const rl::ShaderMacros& Macros)
{
    const std::string ShaderPath = Path.ToString();

    rl::GraphicsPipelineStateDesc PSODesc = {};
    PSODesc.RasterizerDesc(rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::BACK)
        .DepthDesc(true, rl::ComparisionFunc::LESS_EQUAL)
        .TargetBlendDesc(SpaceRenderer_c::GetMaterialPipelineTargetDesc())
        .VertexShader(rl::CreateVertexShader(ShaderPath.c_str(), Macros))
        .PixelShader(rl::CreatePixelShader(ShaderPath.c_str(), Macros))
        .RootSignature(SpaceRenderer_c::GetRootSignature());

    return PSODesc;
}

TextureIndex MaterialShader_c::GetTextureBindIndex(int TextureID) const
{
    if (TextureID >= 0 && BoundTextures.size() > TextureID)
    {
        if (BoundTextures[TextureID])
        {
            return rl::GetDescriptorIndex(BoundTextures[TextureID]->SRV);
        }
    }
    ENSUREMSG(false, "[MaterialShader_c::GetTextureBindIndex] Failed to find a valid texture binding for material");
    return 0u;
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
        .TargetBlendDesc(SpaceRenderer_c::GetMaterialPipelineTargetDesc())
        .VertexShader(rl::CreateVertexShader(ShaderPath.c_str()))
        .PixelShader(rl::CreatePixelShader(ShaderPath.c_str()))
        .RootSignature(SpaceRenderer_c::GetRootSignature());

    PSODesc.DebugName = L"DefaultMaterialShader";
    PSO = rl::CreateGraphicsPipelineState(PSODesc);

    // A mirroring transform reverses triangle winding, so those draws need the opposite cull face.
    PSODesc.Cull = rl::CullMode::FRONT;
    PSODesc.DebugName = L"DefaultMaterialShaderMirrored";
    PSOMirrored = rl::CreateGraphicsPipelineState(PSODesc);

    return PSO.IsValid() && PSOMirrored.IsValid();
}

rl::GraphicsPipelineState_t DefaultMaterialShader_c::GetPSO(bool Mirrored)
{
    return Mirrored ? PSOMirrored : PSO;
}

void DefaultMaterialShader_c::GetDefaultParams(std::vector<uint8_t>& OutData) const
{
    OutData.resize(sizeof(Parameters_s));
	Parameters_s* Params = reinterpret_cast<Parameters_s*>(OutData.data());
	Params->Color = float3(0.5f);
}

bool ErrorMaterialShader_c::Compile()
{
    const std::string ShaderPath = Path_s(PathDirectory_e::Shaders, L"Game", L"ErrorMaterial.hlsl").ToString();

    rl::GraphicsPipelineStateDesc PSODesc = {};
    PSODesc.RasterizerDesc(rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::BACK)
        .DepthDesc(true, rl::ComparisionFunc::LESS_EQUAL)
        .TargetBlendDesc(SpaceRenderer_c::GetMaterialPipelineTargetDesc())
        .VertexShader(rl::CreateVertexShader(ShaderPath.c_str()))
        .PixelShader(rl::CreatePixelShader(ShaderPath.c_str()))
        .RootSignature(SpaceRenderer_c::GetRootSignature());

    PSODesc.DebugName = L"ErrorMaterialShader";
    PSO = rl::CreateGraphicsPipelineState(PSODesc);

    PSODesc.Cull = rl::CullMode::FRONT;
    PSODesc.DebugName = L"ErrorMaterialShaderMirrored";
    PSOMirrored = rl::CreateGraphicsPipelineState(PSODesc);

    return PSO.IsValid() && PSOMirrored.IsValid();
}

rl::GraphicsPipelineState_t ErrorMaterialShader_c::GetPSO(bool Mirrored)
{
    return Mirrored ? PSOMirrored : PSO;
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

const ShaderParam_s* MaterialShaderInstance_c::FindParam(std::string_view Param) const
{
    if (Parent)
    {
        return Parent->FindParam(Param);
    }
    return nullptr;
}

void MaterialShaderInstance_c::SetDefaultValue(const ShaderParam_s* Param, const void* Data, size_t DataSize)
{
    CHECK(Param->Size == DataSize);
    CHECK(ParamData.size() >= (Param->Offset + Param->Size));
    memcpy(ParamData.data() + Param->Offset, Data, DataSize);
}

void MaterialShaderInstance_c::Deserialize(const JsonValue_s& Data)
{
    auto ParamsIt = Data.Json.find("MaterialParams");
    if (ParamsIt != Data.Json.end() && ParamsIt->is_array())
    {
        for (const Json_t& ParamNode : *ParamsIt)
        {
            std::string ParamName;
            if (!ENSUREMSG(JsonHelpers::ParseString(ParamNode, "Name", ParamName), "[MaterialShaderInstance_c::Deserialize] Material param missing name"))
            {
                continue;
            }

            int32_t ParamTypeRaw = 0;
            if (!ENSUREMSG(JsonHelpers::ParseInt(ParamNode, "Type", ParamTypeRaw), "[MaterialShaderInstance_c::Deserialize Material param missing type"))
            {
                continue;
            }

            const ShaderParamType_e ParamType = static_cast<ShaderParamType_e>(ParamTypeRaw);

            const ShaderParam_s* FoundParam = FindParam(ParamName);
            if (!FoundParam)
            {
                LOGWARNING("[MaterialShaderInstance_c::Deserialize] Material param %s not found in shader", ParamName.c_str());
                continue;
            }

            if (FoundParam->Type != ParamType)
            {
                LOGWARNING("[MaterialShaderInstance_c::Deserialize] Material param %s type %d does not match the type used by the shader", ParamName.c_str(), static_cast<int32_t>(ParamType), static_cast<int32_t>(FoundParam->Type));
                continue;
            }

            bool IsFloatType = false;
            switch (ParamType)
            {
            case ShaderParamType_e::_float:
            case ShaderParamType_e::_float2:
            case ShaderParamType_e::_float3:
            case ShaderParamType_e::_float4:
                IsFloatType = true;
                break;
            default:
                IsFloatType = false;
            }

            if (IsFloatType)
            {
                int32_t Components = 0;
                switch (ParamType)
                {
                case ShaderParamType_e::_float:
                    Components = 1;
                    break;
                case ShaderParamType_e::_float2:
                    Components = 2;
                    break;
                case ShaderParamType_e::_float3:
                    Components = 3;
                    break;
                case ShaderParamType_e::_float4:
                    Components = 4;
                    break;
                }

                if (ENSUREMSG(Components > 0, "[MaterialShaderInstance_c::Deserialize] Failed to parse components from type for %s", ParamName.c_str()))
                {
                    float FloatComponents[4] = {};
                    if (ENSUREMSG(JsonHelpers::ParseFloatComponents(ParamNode, "Value", FloatComponents, Components), "[MaterialShaderInstance_c::Deserialize] Failed to parse value from type for %s", ParamName.c_str()))
                    {
                        SetDefaultValue(FoundParam, FloatComponents, Components * sizeof(float));
                    }
                }

                continue;
            }

            bool IsTextureType = ParamType == ShaderParamType_e::_TextureIndex;
            if (IsTextureType)
            {

                continue;
            }

        }
    }
}

void MaterialShaderInstance_c::Update()
{
    if (ParamData.empty())
        return;

    if (ENSUREMSG(!ConstantBuffer.IsValid(), "[MaterialShaderInstance_c::Update] Cannot update already built instance"))
    {
        ConstantBuffer = rl::CreateConstantBuffer(ParamData.data(), ParamData.size());
    }   
}

rl::GraphicsPipelineState_t MaterialShaderInstance_c::GetPSO(bool Mirrored)
{
    return Parent != nullptr ? Parent->GetPSO(Mirrored) : rl::GraphicsPipelineState_t::INVALID;
}

rl::ConstantBuffer_t MaterialShaderInstance_c::GetConstantBuffer()
{
    return ConstantBuffer;
}

void MaterialShaderInstanceDynamic_c::SetValue(const ShaderParam_s* Param, const void* Data, size_t DataSize)
{
    CHECK(Param->Size == DataSize);
    CHECK(ParamData.size() >= (Param->Offset + Param->Size));
    memcpy(ParamData.data() + Param->Offset, Data, DataSize);
}


void MaterialShaderInstanceDynamic_c::SetFloat(std::string_view Param, float Value)
{
    if (const ShaderParam_s* Found = FindParam(Param))
    {
        SetValue(Found, &Value, sizeof(Value));
    }
}

void MaterialShaderInstanceDynamic_c::SetFloat2(std::string_view Param, float2 Value)
{
    if (const ShaderParam_s* Found = FindParam(Param))
    {
        SetValue(Found, &Value, sizeof(Value));
    }
}

void MaterialShaderInstanceDynamic_c::SetFloat3(std::string_view Param, float3 Value)
{
    if (const ShaderParam_s* Found = FindParam(Param))
    {
        SetValue(Found, &Value, sizeof(Value));
    }
}

void MaterialShaderInstanceDynamic_c::SetFloat4(std::string_view Param, float4 Value)
{
    if (const ShaderParam_s* Found = FindParam(Param))
    {
        SetValue(Found, &Value, sizeof(Value));
    }
}
