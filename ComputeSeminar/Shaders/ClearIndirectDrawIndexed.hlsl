#include "IndirectArguments.h"
#include "Scene.h"

RWStructuredBuffer<IndirectDrawIndexedLayout> u_IndirectDrawIndexedLayouts[512] : register(u0, space0);
ConstantBuffer<SceneData> c_SceneData : register(b0, space0);

[NumThreads(1, 1, 1)]
void main()
{
    u_IndirectDrawIndexedLayouts[c_SceneData.IndirectDrawUAVIndex][0u].NumInstances = 0u;
}