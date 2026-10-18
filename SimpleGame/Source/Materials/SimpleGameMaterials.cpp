#include "SimpleGameMaterials.h"

#include <Assets/MaterialManager.h>
#include <Rendering/Materials.h>

class SkyMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        TextureIndex SkyTexture;
        float Luminance = 8000.0f;
        float2 __Pad;
    };

public:

    SkyMaterialShader_c()
    {
        ShaderFilePath = ShaderPath_s(L"Materials/Sky.hlsl");
        ShaderDebugName = L"SkyShader";

        ASSIGN_SHADER_PARAM(TextureIndex, SkyTexture);
        ASSIGN_SHADER_PARAM(float, Luminance);
    }

    virtual void Load() override
    {
        LoadShaderTexture("SkyTexture", AssetPath_s(L"Textures/SkyEnv.hp_tex"));
    }

    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override { InitDefaultParams<Parameters_s>(OutData); }
};

void RegisterMaterials()
{
    MaterialManager::RegisterMaterialShaderClass<SkyMaterialShader_c>(L"SkyMaterialShader");
}
