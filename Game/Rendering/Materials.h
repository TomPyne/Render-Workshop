#pragma once

#include <Render/RenderTypes.h>

#include <map>
#include <SurfMath.h>

enum class MaterialVertexFlags_e : uint32_t
{
	HAS_POSITION = 0, // Will always have position
	HAS_UV = 1 << 0,
	HAS_NORMAL = 1 << 1,
	HAS_TANGENTS = 1 << 2,
	HAS_COLOR = 1 << 3,
	COUNT
};
IMPLEMENT_FLAGS(MaterialVertexFlags_e, uint32_t)

class Material_c
{
public:

	virtual ~Material_c() = default;

	virtual const char* GetShaderPath() const = 0;
	virtual rl::GraphicsPipelineStatePtr GetPSO(MaterialVertexFlags_e VertexFlags) const = 0;

	const rl::GraphicsPipelineState_t FindVertexPSO(MaterialVertexFlags_e VertexFlags) const;

	void CacheVertexPSO(MaterialVertexFlags_e VertexFlags, const rl::GraphicsPipelineStatePtr& PSO) const;

	std::map<MaterialVertexFlags_e, rl::GraphicsPipelineStatePtr> VertexPSOMap;
};

class SimpleMaterial_c : public Material_c
{
public:

	virtual ~SimpleMaterial_c() = default;

	virtual const char* GetShaderPath() const override;
	virtual rl::GraphicsPipelineStatePtr GetPSO(MaterialVertexFlags_e VertexFlags) const override;
};

class BasicMaterial_c
{
public:
	rl::GraphicsPipelineStatePtr PSO;
	rl::ConstantBufferPtr MaterialConstants;
};

BasicMaterial_c* MakeBasicMaterial(float3 Color);
void DestroyBasicMaterial(BasicMaterial_c* Material);
