#pragma once

#include <Render/RenderTypes.h>

#include <unordered_map>
#include <SurfMath.h>

#define SHADER_PARAM(Type, Name) { offsetof(Parameters_s, Name), sizeof(Type) }

class MaterialShader_c
{
public:

	struct ShaderParam_s
	{
		ShaderParam_s() = default;
		ShaderParam_s(size_t InOffset, size_t InSize)
			: Offset(static_cast<uint16_t>(InOffset))
			, Size(static_cast<uint16_t>(InSize))
		{}
		uint16_t Offset = 0u;
		uint16_t Size = 0u;
	};

	virtual ~MaterialShader_c() = default;

	virtual void Init() {}
	virtual rl::GraphicsPipelineState_t GetPSO();

	virtual uint32_t GetShaderParamBufferSizeFloats() const { return 0u; }
	virtual uint32_t GetShaderParamBufferSize() const;

	const ShaderParam_s* GetShaderParam(const std::string& Param) const;

protected:

	std::unordered_map<std::string, ShaderParam_s> ShaderParameters;
};

class DefaultMaterialShader_c : public MaterialShader_c
{
	struct Parameters_s
	{
		float3 Color;
		float __Pad;
	};
public:

	virtual ~DefaultMaterialShader_c() = default;

	virtual void Init() override;
	virtual rl::GraphicsPipelineState_t GetPSO() override;
	virtual uint32_t GetShaderParamBufferSize() const override { return 4u * sizeof(float); }

private:
	rl::GraphicsPipelineStatePtr PSO;
};

class MaterialShaderInstance_c
{
public:
	void SetParent(const std::shared_ptr<MaterialShader_c>& InParent);

	void SetFloat(const std::string& Param, float Value);
	void SetFloat2(const std::string& Param, float2 Value);
	void SetFloat3(const std::string& Param, float3 Value);
	void SetFloat4(const std::string& Param, float4 Value);

	const MaterialShader_c::ShaderParam_s* FindParam(const std::string& Param) const;

private:
	std::shared_ptr<MaterialShader_c> Parent;
	std::vector<uint8_t> ParamData;
};

class BasicMaterial_c
{
public:
	rl::GraphicsPipelineStatePtr PSO;
	rl::ConstantBufferPtr MaterialConstants;
};

BasicMaterial_c* MakeBasicMaterial(float3 Color);
void DestroyBasicMaterial(BasicMaterial_c* Material);

struct Shader_s
{
	bool Ready = false;

	rl::GraphicsPipelineStatePtr PSO;
};

struct Material_s
{
	bool Ready = false;

	std::shared_ptr<Shader_s> Shader;
	float3 Color = float3(1.0f, 1.0f, 1.0f);
};
