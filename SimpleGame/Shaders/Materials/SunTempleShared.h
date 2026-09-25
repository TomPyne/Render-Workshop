#pragma once

float2 Panner2(float2 Input, float Time, float2 Speed)
{
    return Input + Time * Speed;
}

float3 BlendDetailNormals(float3 Base, float3 Detail)
{
    Base = float3(Base.x, Base.y, Base.z + 1.0f);
    Detail = float3(-Detail.x, -Detail.y, Detail.z);
    return (Base * dot(Base, Detail)) - (Base.z * Detail); 
}

float3 SafeNormalize(float3 Input)
{
    float A = dot(Input, Input);
    float B = 0.000001;
    return A > B ? normalize(Input) : float3(0.0f, 0.0f, 0.0f);
}

float GetPixelDistance(float4 SvPosition)
{
    float2 Ndc = SvPosition.xy * c_View.InvViewportSize * 2.0f - 1.0f;
    float2 ProjScale = float2(length(c_View.ViewProjectionMatrix[0].xyz), length(c_View.ViewProjectionMatrix[1].xyz));
    return SvPosition.w * length(float3(Ndc / ProjScale, 1.0f));
}
