#pragma once

#include "Rendering/Materials.h"

class BRDFMaterialShader_c : public MaterialShader_c
{
	struct Parameters_s
	{
		float3 Albedo;
		float __Pad;
	};
public:

	BRDFMaterialShader_c();
	virtual ~BRDFMaterialShader_c() = default;

	virtual bool Compile() override;
	virtual rl::GraphicsPipelineState_t GetPSO(bool Mirrored) override;
	virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
	virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;

private:
	rl::GraphicsPipelineStatePtr PSO;
	rl::GraphicsPipelineStatePtr PSOMirrored;
};