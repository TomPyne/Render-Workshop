struct ConstantData
{
    float4x4 Projection;
    float4x4 InverseProjection;
    float4x4 View;

    uint DepthTextureIndex;
    uint NormalTextureIndex;
    uint OutputTextureIndex;
    float __pad0;

    float2 ViewportSizeRcp;
    uint2 ViewportSize;
};
ConstantBuffer<ConstantData> c_G : register(b1);

Texture2D<float> t_tex2d_f1[8192] : register(t0, space0); // Depth 
Texture2D<float4> t_tex2d_f4[8192] : register(t0, space1); // Normal
RWTexture2D<float> u_tex2d_f1[8192] : register(u0, space0); // Output

float3 GetViewPosFromScreen(float4x4 InverseProjection, float2 ScreenPos, float Depth)
{
    float4 Projected = float4(ScreenPos, Depth, 1.0f);
    float4 Unprojected = mul(InverseProjection, Projected);
    return Unprojected.xyz / Unprojected.w;
}

float3 GetViewPosFromScreen(float2 Pixel)
{
    float Depth = t_tex2d_f1[c_G.DepthTextureIndex].Load(uint3(Pixel, 0));
    float2 NDC = (Pixel * c_G.ViewportSizeRcp) * 2.0f - 1.0f;
    return GetViewPosFromScreen(c_G.InverseProjection, NDC.xy, Depth);
}

float Random(float3 Seed)
{
    return frac(sin(dot(Seed.xyz, float3(12.9898,78.233, 128.943)))* 43758.5453123);
}

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    const float MaxDistance = 15.0f;
    const float Resolution = 0.3f;
    int Steps = 10;
    const float Thickness = 0.5f;
    const float Pi = 3.141593;

    float3 Normal = t_tex2d_f4[c_G.NormalTextureIndex].Load(uint3(DispatchThreadId.xy, 0)).xyz;

    float2 StartPixel = float2(DispatchThreadId.xy) + 0.5;

    float3 PositionFrom = GetViewPosFromScreen(StartPixel);
    float3 Direction = normalize(PositionFrom);

    float Theta = Random(float3(DispatchThreadId % 4)) * 2.0f * Pi;
    float Phi = (Random(float3(DispatchThreadId % 4)) + (0.5f * Pi)) - (0.5f * Pi);
    float3 RandomHemi = float3(sin(Phi) * cos(Theta), sin(Phi) * sin(Theta), cos(Theta));

    float3 Tangent = normalize(RandomHemi - Normal * dot(RandomHemi, Normal));
    float3 Bitangent = cross(Normal, Tangent);

    float3 SampleDir = mul(mat3(Tangent, Bitangent, Normal), RandomHemi);
    float3 SampleDirView = mul(View, float4(SampleDir, 1.0f)).xyz;

    float3 PositionTo = PositionFrom + SampleDirView * MaxDistance;
    
    float4 EndPixel = mul(c_G.Projection, float4(PositionTo, 1.0f));
    EndPixel.xyz /= EndPixel.w;
    //EndPixel.xy = EndPixel.xy * 0.5f + 0.5f;
    EndPixel.xy *= c_G.ViewportSize;

    float DeltaX = EndPixel.x - StartPixel.x;
    float DeltaY = EndPixel.y - StartPixel.y;

    bool UseX = abs(DeltaX) >= abs(DeltaY);
    float Delta = UseX ? abs(DeltaX) : abs(DeltaY);
    Delta *= saturate(Resolution);

    float2 Increment = float2(DeltaX, DeltaY) / max(Delta, 0.001f);

    float Search0 = 0.0f;
    float Search1 = 0.0f;
    bool Hit0 = false;
    bool Hit1 = false;

    float ViewDistance = PositionFrom.z;
    float TraceDepth = Thickness;

    float2 TracePixel = StartPixel;

    for(int i = 0; i < int(Delta); i++)
    {
        TracePixel += Increment;
        PositionTo = GetViewPosFromScreen(TracePixel);
        Search1 = UseX ? (TracePixel.x - StartPixel.x) / DeltaX : (TracePixel.y - StartPixel.y) / DeltaY;

        ViewDistance = (PositionFrom.y * PositionTo.y) / lerp(PositionTo.y, PositionFrom.y, Search1);

        TraceDepth = ViewDistance - PositionTo.y;

        if(TraceDepth > 0 && TraceDepth < Thickness)
        {
            Hit0 = true;
            break;
        }
        else
        {
            Search0 = Search1;
        }

        Search1 = Search0 + ((Search1 - Search0))
    }

    if(Hit0)
    {
        for(int i = 0; i < Steps; i++)
        {
            TracePixel = lerp(StartPixel, EndPixel, Search1);
            PositionTo = GetViewPosFromScreen(TracePixel);

            ViewDistance = (PositionFrom.y * PositionTo.y) / lerp(PositionTo.y, PositionFrom.y, Search1);
            TraceDepth = ViewDistance - PositionTo.y;

            if(TraceDepth > 0 && TraceDepth < Thickness)
            {
                Hit1 = true;
                Search1 = Search0 + ((Search1 - Search0) / 2.0f);
            }
            else
            {
                float Temp = Search1;
                Search1 = Search1 + ((Search1 - Search0) / 2.0f);
                Search0 = Temp;
            }
        }
    }

    float Visibility = 0.0f;
    if(Hit1 && all(int2(Pixel) >= 0) && all(int2(Pixel) < c_G.ViewportSize))
    {
        Visibility *= PositionTo.w;
        Visibility *= 1.0f - max(dot(Direction, SampleDirView), 0.0f);
        Visibility *= 1.0f - saturate(TraceDepth /  Thickness);
        Visibility *= 1.0f - saturate(length(PositionTo - PositionFrom) / MaxDistance);

        Visibility = saturate(Visibility);
    }

    u_tex2d_f1[c_G.OutputTextureIndex][DispatchThreadId.xy] = Visibility;
}