#pragma once

#include <Rendering/Materials.h>

class BackgroundMatteMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float3 Color;
        float Contrast;

        float Brightness;
        TextureIndex MatteTextureIndex;
        float2 __Pad;
    };

public:

    BackgroundMatteMaterialShader_c();
    virtual ~BackgroundMatteMaterialShader_c() = default;

    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};