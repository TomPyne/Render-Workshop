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

	virtual bool Compile() { return true; }
	virtual rl::GraphicsPipelineState_t GetPSO();
	virtual rl::ConstantBuffer_t GetConstantBuffer();

	virtual uint32_t GetShaderParamBufferSizeFloats() const { return 0u; }
	virtual uint32_t GetShaderParamBufferSize() const;

	virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const {}

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

	DefaultMaterialShader_c();
	virtual ~DefaultMaterialShader_c() = default;

	virtual bool Compile() override;
	virtual rl::GraphicsPipelineState_t GetPSO() override;
	virtual uint32_t GetShaderParamBufferSize() const override { return 4u * sizeof(float); }
	virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const;

private:
	rl::GraphicsPipelineStatePtr PSO;
};

class MaterialShaderInstance_c
{
public:
	~MaterialShaderInstance_c();

	void SetParent(const std::shared_ptr<MaterialShader_c>& InParent);

	void SetValue(const MaterialShader_c::ShaderParam_s* Param, const void* Data, size_t DataSize);

	void SetFloat(const std::string& Param, float Value);
	void SetFloat2(const std::string& Param, float2 Value);
	void SetFloat3(const std::string& Param, float3 Value);
	void SetFloat4(const std::string& Param, float4 Value);
	const MaterialShader_c::ShaderParam_s* FindParam(const std::string& Param) const;

	void Update();

	rl::GraphicsPipelineState_t GetPSO();
	rl::ConstantBuffer_t GetConstantBuffer();

private:
	std::shared_ptr<MaterialShader_c> Parent;
	std::vector<uint8_t> ParamData;
	rl::ConstantBufferPtr ConstantBuffer;
};
