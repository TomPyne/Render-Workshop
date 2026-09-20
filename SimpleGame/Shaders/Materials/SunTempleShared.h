#pragma once


float3 BlendDetailNormals(float3 Base, float3 Detail)
{
    Base = float3(Base.x, Base.y, Base.z + 1.0f);
    Detail = float3(-Detail.x, -Detail.y, Detail.z);
    return (Base * dot(Base, Detail)) - (Base.z * Detail); 
}