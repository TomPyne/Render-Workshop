#include "Materials.h"

#include <Render/Render.h>

// ## BasicShader
// BasicShader contains the shaders, PSOs, and buffers

const rl::GraphicsPipelineState_t Material_c::FindVertexPSO(MaterialVertexFlags_e VertexFlags) const
{
   auto FoundIt = VertexPSOMap.find(VertexFlags);

   if (FoundIt != VertexPSOMap.end())
       return FoundIt->second;

   return {};
}

void Material_c::CacheVertexPSO(MaterialVertexFlags_e VertexFlags, const rl::GraphicsPipelineStatePtr& PSO) const
{
    const_cast<Material_c*>(this)->VertexPSOMap.insert(std::make_pair(VertexFlags, PSO));
}

const char* SimpleMaterial_c::GetShaderPath() const
{
    return "Game/Shaders/SimpleMaterial.hlsl";
}

rl::GraphicsPipelineStatePtr SimpleMaterial_c::GetPSO(MaterialVertexFlags_e VertexFlags) const
{
    if (VertexFlags != MaterialVertexFlags_e::HAS_POSITION)
    {
        return {};
    }
    
    const rl::GraphicsPipelineState_t Found = FindVertexPSO(VertexFlags);
    if (rl::IsValid(Found))
    {
        return Found;
    }

    // Create and cache

    rl::VertexShader_t MeshVS = rl::CreateVertexShader(GetShaderPath());
    rl::PixelShader_t MeshPS = rl::CreatePixelShader(GetShaderPath());

    rl::GraphicsPipelineStateDesc PsoDesc = {};
    PsoDesc.RasterizerDesc(rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::BACK)
        .DepthDesc(true, rl::ComparisionFunc::LESS_EQUAL)
        .TargetBlendDesc({ rl::RenderFormat::R16G16B16A16_FLOAT, rl::RenderFormat::R16G16B16A16_FLOAT, rl::RenderFormat::R16G16_FLOAT, rl::RenderFormat::R16G16_FLOAT }, { rl::BlendMode::None(), rl::BlendMode::None(), rl::BlendMode::None(), rl::BlendMode::None() }, rl::RenderFormat::D32_FLOAT)
        .VertexShader(MeshVS)
        .PixelShader(MeshPS);

    PsoDesc.DebugName = L"SimpleMaterialPSO";
    rl::GraphicsPipelineStatePtr PSO = CreateGraphicsPipelineState(PsoDesc);

    CacheVertexPSO(VertexFlags, PSO);

    return PSO;
}

BasicMaterial_c* MakeBasicMaterial(float3 Color)
{
    BasicMaterial_c* NewMaterial = new BasicMaterial_c;
    static const char* ShaderPath = "Game/Shaders/BasicMaterial.hlsl";
    rl::VertexShader_t MeshVS = rl::CreateVertexShader(ShaderPath);
    rl::PixelShader_t MeshPS = rl::CreatePixelShader(ShaderPath);

    rl::GraphicsPipelineStateDesc PsoDesc = {};
    PsoDesc.RasterizerDesc(rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::BACK)
        .DepthDesc(true, rl::ComparisionFunc::LESS_EQUAL)
        .TargetBlendDesc({ rl::RenderFormat::R16G16B16A16_FLOAT, rl::RenderFormat::R16G16B16A16_FLOAT, rl::RenderFormat::R16G16_FLOAT, rl::RenderFormat::R16G16_FLOAT }, { rl::BlendMode::None(), rl::BlendMode::None(), rl::BlendMode::None(), rl::BlendMode::None() }, rl::RenderFormat::D32_FLOAT)
        .VertexShader(MeshVS)
        .PixelShader(MeshPS);

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
