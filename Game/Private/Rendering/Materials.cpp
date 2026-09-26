#include "Rendering/Materials.h"

#include "Rendering/SpaceRenderer.h"
#include "Rendering/Texture.h"

#include <Assets/TextureManager.h>
#include <Render/Render.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

bool MaterialShader_c::Compile()
{
    if (ShaderFilePath.empty())
    {
        LOGWARNING("[MaterialShader_c::Compile] Failed to compile due to ShaderFilePath being empty");
        return false;
    }

    rl::GraphicsPipelineStateDesc PSODesc = MakeDefaultPSODesc(Path_s(PathDirectory_e::Shaders, ShaderFilePath), {});

    PSODesc.DebugName = ShaderDebugName;
    PSO = rl::CreateGraphicsPipelineState(PSODesc);

    PSODesc.Cull = rl::CullMode::FRONT;
    PSODesc.DebugName = ShaderDebugName + L"Mirrored";
    PSOMirrored = rl::CreateGraphicsPipelineState(PSODesc);

    return PSO.IsValid() && PSOMirrored.IsValid();
}

rl::GraphicsPipelineState_t MaterialShader_c::GetPSO(bool Mirrored)
{
    return Mirrored ? PSOMirrored : PSO;
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

bool MaterialShader_c::IsReady() const
{
    if (Ready)
        return true;

    for (const ShaderTexture_s& Tex : ShaderTextures)
    {
        if (Tex.Texture && !Tex.Texture->IsReady())
        {
            return false;
        }
    }

    Ready = true;
    return true;
}

void MaterialShader_c::AssignTextureParams(void* const ParamData, size_t ParamSize) const
{
    for (const ShaderTexture_s& Tex : ShaderTextures)
    {
#ifndef NDEBUG
        CHECK(Tex.Texture != nullptr);
        CHECK(Tex.Texture->IsReady());
        CHECK(Tex.Texture->Texture.IsValid());
        CHECK(Tex.Texture->SRV.IsValid());
        CHECK(ParamSize >= Tex.ParamOffset + sizeof(uint32_t));
#endif
        const uint32_t DescriptorIndex = rl::GetDescriptorIndex(Tex.Texture->SRV);
        CHECK(DescriptorIndex != 0);
        memcpy((uint8_t*)ParamData + Tex.ParamOffset, &DescriptorIndex, sizeof(DescriptorIndex));
    }
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

void MaterialShader_c::LoadShaderTexture(std::string_view ParamName, const Path_s& Path)
{
    const ShaderParam_s* Param = FindParam(ParamName);
    if (ENSUREMSG(Param != nullptr, "MaterialShader_c::LoadShaderTexture passed invalid param name %s", std::string(ParamName).c_str()))
    {
        ShaderTexture_s ShaderTexture;
        ShaderTexture.Texture = TextureManager::RequestTexture(Path);
        ShaderTexture.ParamOffset = Param->Offset;
        
        if (ENSUREMSG(ShaderTexture.Texture != nullptr, "[MaterialShader_c::LoadShaderTexture] Failed to load texture %s for param", Path.ToString().c_str(), std::string(ParamName).c_str()))
        {
            ShaderTextures.push_back(ShaderTexture);
        }        
    }    
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
    OverrideParamData.clear();
    ConstantBuffer = {};

    if (Parent)
    {
        OverrideParamData.resize(Parent->GetShaderParamBufferSize());
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
    CHECK(Param);
    CHECK(Param->Size == DataSize);
    CHECK(OverrideParamData.size() >= (Param->Offset + Param->Size));
    memcpy(OverrideParamData.data() + Param->Offset, Data, DataSize);

    // You can technically push duplicates but whatever gets pushed last always wins so not my problem
    OverridenParams.push_back(*Param);
}

void MaterialShaderInstance_c::LoadOverrideShaderTexture(std::string_view ParamName, const Path_s& Path)
{
    const ShaderParam_s* Param = FindParam(ParamName);
    if (ENSUREMSG(Param != nullptr, "[MaterialShaderInstance_c::LoadOverrideShaderTexture] passed invalid param name %s", ParamName))
    {
        ShaderTexture_s ShaderTexture;
        ShaderTexture.Texture = TextureManager::RequestTexture(Path);
        ShaderTexture.ParamOffset = Param->Offset;

        if (ENSUREMSG(ShaderTexture.Texture != nullptr, "[MaterialShaderInstance_c::LoadOverrideShaderTexture] Failed to load texture %s for param", Path.ToString().c_str(), ParamName))
        {
            OverrideShaderTextures.push_back(ShaderTexture);
        }
    }
}

void MaterialShaderInstance_c::AssignOverrideTextureParams(void* const ParamData, size_t ParamSize) const
{
    for (const ShaderTexture_s& Tex : OverrideShaderTextures)
    {
#ifndef NDEBUG
        CHECK(Tex.Texture != nullptr);
        CHECK(Tex.Texture->IsReady());
        CHECK(Tex.Texture->Texture.IsValid());
        CHECK(Tex.Texture->SRV.IsValid());
        CHECK(ParamSize >= Tex.ParamOffset + sizeof(uint32_t));
#endif
        const uint32_t DescriptorIndex = rl::GetDescriptorIndex(Tex.Texture->SRV);
        CHECK(DescriptorIndex != 0);
        memcpy((uint8_t*)ParamData + Tex.ParamOffset, &DescriptorIndex, sizeof(DescriptorIndex));
    }
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
                std::wstring TexAssetPath;
                if (ENSUREMSG(JsonHelpers::ParseWString(ParamNode, "Value", TexAssetPath), "[MaterialShaderInstance_c::Deserialize] Failed to parse value from type for %s", ParamName.c_str()))
                {
                    LoadOverrideShaderTexture(ParamName, Path_s::FromTokenised(TexAssetPath));
                }

                continue;
            }

            bool IsDynamicBool = ParamType == ShaderParamType_e::_DynamicBool;
            if (IsDynamicBool)
            {
                bool Value;
                if (ENSUREMSG(JsonHelpers::ParseBool(ParamNode, "Value", Value), "[MaterialShaderInstance_c::Deserialize] Failed to parse value from type for %s", ParamName.c_str()))
                {
                    uint32_t ValueUint = Value ? 1u : 0u;
                    SetDefaultValue(FoundParam, &ValueUint, sizeof(ValueUint));
                }
                continue;
            }

            ENSUREMSG(false, "[MaterialShaderInstance_c::Deserialize] Unsupported type for %s", ParamName.c_str());
        }
    }
}

void MaterialShaderInstance_c::Update()
{
    if (!Parent)
        return;

    std::vector<uint8_t> ParamData(Parent->GetShaderParamBufferSize());
    Parent->GetDefaultParams(ParamData);

    for (const ShaderParam_s& OverrideParam : OverridenParams)
    {
        memcpy(ParamData.data() + OverrideParam.Offset, OverrideParamData.data() + OverrideParam.Offset, OverrideParam.Size);
    }

    AssignOverrideTextureParams(ParamData.data(), ParamData.size());

    ConstantBuffer = rl::CreateConstantBuffer(ParamData.data(), ParamData.size());
}

bool MaterialShaderInstance_c::IsReady() const
{
    if (Ready)
        return true;

    if (!Parent)
    {
        return false;
    }

    if (!Parent->IsReady())
        return false;

    for (const ShaderTexture_s& Tex : OverrideShaderTextures)
    {
        if (Tex.Texture && !Tex.Texture->IsReady())
        {
            return false;
        }
    }

    Ready = true;
    return true;
}

rl::GraphicsPipelineState_t MaterialShaderInstance_c::GetPSO(bool Mirrored)
{
    return Parent != nullptr ? Parent->GetPSO(Mirrored) : rl::GraphicsPipelineState_t::INVALID;
}

rl::ConstantBuffer_t MaterialShaderInstance_c::GetConstantBuffer()
{
    CHECK(IsReady());
    if (!ConstantBuffer.IsValid())
    {
        Update();
    }
    return ConstantBuffer;
}

void MaterialShaderInstanceDynamic_c::SetValue(const ShaderParam_s* Param, const void* Data, size_t DataSize)
{
    CHECK(Param);
    CHECK(Param->Size == DataSize);
    CHECK(DynamicOverrideParamData.size() >= (Param->Offset + Param->Size));
    memcpy(DynamicOverrideParamData.data() + Param->Offset, Data, DataSize);

    DynamicOverridenParams.push_back(*Param);
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
