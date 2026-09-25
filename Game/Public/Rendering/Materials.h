#pragma once

#include <Render/RenderTypes.h>

#include <memory>
#include <unordered_map>
#include <SurfMath.h>

struct Path_s;
struct Texture_s;

#define SHADER_PARAM(Type, Name) { offsetof(Parameters_s, Name), sizeof(Type), ShaderParamType_e::_##Type }
#define ASSIGN_SHADER_PARAM(Type, Name) ShaderParameters[#Name] = SHADER_PARAM(Type, Name)

using TextureIndex = uint32_t;
using DynamicBool = uint32_t;

enum class ShaderParamType_e : uint8_t
{
	Unknown = 0,
	_float,				// 1
	_float2,			// 2
	_float3,			// 3
	_float4,			// 4
	_TextureIndex,		// 5
	_DynamicBool,		// 6
	Count
};

struct ShaderParam_s
{
	ShaderParam_s() = default;
	ShaderParam_s(size_t InOffset, size_t InSize, ShaderParamType_e InType)
		: Offset(static_cast<uint16_t>(InOffset))
		, Size(static_cast<uint8_t>(InSize))
		, Type(InType)
	{}
	uint16_t Offset = 0u;
	uint8_t Size = 0u;
	ShaderParamType_e Type = ShaderParamType_e::Unknown;
};

class MaterialShader_c
{
public:

	virtual ~MaterialShader_c() = default;

	virtual void Load() {}
	virtual bool Compile();
	virtual rl::GraphicsPipelineState_t GetPSO(bool Mirrored);
	virtual rl::ConstantBuffer_t GetConstantBuffer();

	virtual uint32_t GetShaderParamBufferSizeFloats() const { return 0u; }
	virtual uint32_t GetShaderParamBufferSize() const;

	virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const {}

	const ShaderParam_s* FindParam(std::string_view Param) const;

protected:

	rl::GraphicsPipelineStateDesc MakeDefaultPSODesc(const Path_s& ShaderPath, const rl::ShaderMacros& Macros);

	void AddBindTexture(int TextureId, const std::shared_ptr<Texture_s>& Texture);
	TextureIndex GetTextureBindIndex(int TextureID) const;

	std::unordered_map<std::string, ShaderParam_s> ShaderParameters;
	std::vector<uint8_t> DefaultParamData;

	std::vector<std::shared_ptr<Texture_s>> BoundTextures;

	rl::GraphicsPipelineStatePtr PSO;
	rl::GraphicsPipelineStatePtr PSOMirrored;

	std::wstring ShaderDebugName;
	std::wstring ShaderFilePath;
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
	virtual rl::GraphicsPipelineState_t GetPSO(bool Mirrored) override;
	virtual uint32_t GetShaderParamBufferSize() const override { return 4u * sizeof(float); }
	virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;

private:
	rl::GraphicsPipelineStatePtr PSO;
	rl::GraphicsPipelineStatePtr PSOMirrored;
};

class ErrorMaterialShader_c : public MaterialShader_c
{
public:
	virtual bool Compile() override;
	virtual rl::GraphicsPipelineState_t GetPSO(bool Mirrored) override;

private:
	rl::GraphicsPipelineStatePtr PSO;
	rl::GraphicsPipelineStatePtr PSOMirrored;
};

class MaterialShaderInstance_c
{
public:
	virtual ~MaterialShaderInstance_c();

	void SetParent(const std::shared_ptr<MaterialShader_c>& InParent);

	const ShaderParam_s* FindParam(std::string_view  Param) const;	

	void Deserialize(const struct JsonValue_s& Data);

	void Update();

	rl::GraphicsPipelineState_t GetPSO(bool Mirrored);
	rl::ConstantBuffer_t GetConstantBuffer();

protected:

	void SetDefaultValue(const ShaderParam_s* Param, const void* Data, size_t DataSize);

	std::shared_ptr<MaterialShader_c> Parent;
	std::vector<uint8_t> ParamData;
	rl::ConstantBufferPtr ConstantBuffer;

	std::vector<std::shared_ptr<Texture_s>> BoundTextures;
};

class MaterialShaderInstanceDynamic_c : public MaterialShaderInstance_c
{
public:
	~MaterialShaderInstanceDynamic_c();

	void SetValue(const ShaderParam_s* Param, const void* Data, size_t DataSize);
	void SetFloat(std::string_view Param, float Value);
	void SetFloat2(std::string_view Param, float2 Value);
	void SetFloat3(std::string_view Param, float3 Value);
	void SetFloat4(std::string_view Param, float4 Value);
};
