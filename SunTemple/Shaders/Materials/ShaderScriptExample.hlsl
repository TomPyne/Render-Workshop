// #define USE_NORMAL 1
// #define USE_UV0 1
// #define USE_UV1 1
// #define USE_CUSTOM_UV1 1

#include "MaterialBaseShared.h"

float DirtBrightness;
float DirtContrast;
float ScaleDirt;
float AORoughnessIntensity;

float3 ColorMetal;
float RoughnessMetalHigh;

float RoughnessMetalLow;
uint AlbedoTexture;
uint MaskTexture;
uint NormalTexture;

void VertexShaderMain(in uint VertexID, inout PixelShaderMaterialOutputs_s Output)
{
    SetUV1(Output.UV0 * ScaleDirt);
}

void PixelShaderMain(in Interpolants_s Input, out PSOutput_s Output)
{
    float Mask = SampleWrapped(MaskTexture, Input.UV0).r;
    float AlbedoAlpha = SampleWrapped(AlbedoTexture, Input.UV0).r;
    float AO = saturate(Mask + AORoughnessIntensity);
    float DirtMask = saturate(pow(AlbedoAlpha, DirtContrast) * DirtBrightness);

    Output.Albedo = ColorMetal * Mask;
    Output.Metallic = 1.0f;
    Output.Roughness = lerp(RoughnessMetalHigh, RoughnessMetalLow, AO * DirtMask);
    Output.TangentNormal = SampleNormalWrapped(NormalTexture, Input.UV0);
}