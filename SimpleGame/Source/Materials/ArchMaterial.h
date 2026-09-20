#pragma once

#include <Rendering/Materials.h>

class ArchMaterialShader_c : public MaterialShader_c
{
	struct Parameters_s
	{
        float3 ColorMarble1;
        float AOStrength;

        float3 ColorMarble2;
        float AOStrength2;

        float NormalIntensity;
        float RoughnessMarble1;
        float RoughnessMarble2;
        float ScaleMarble1;

        float ScaleMarble2;
        float3 _Pad0;

        TextureIndex MaskTextureIndex;
        TextureIndex AlbedoTextureIndex;
        TextureIndex NormalTextureIndex;
        TextureIndex DetailNormalTextureIndex;
	};
public:

	ArchMaterialShader_c();
	virtual ~ArchMaterialShader_c() = default;

    virtual void Load() override;
	virtual bool Compile() override;
	virtual rl::GraphicsPipelineState_t GetPSO(bool Mirrored) override;
	virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
	virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;

private:
	rl::GraphicsPipelineStatePtr PSO;
	rl::GraphicsPipelineStatePtr PSOMirrored;
};