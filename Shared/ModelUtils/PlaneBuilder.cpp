#include "PlaneBuilder.h"

namespace PlaneBuilder
{
    static const float3 PlanePositions[] = {
        float3(-1.0f, 0.0f, -1.0f),
        float3(-1.0f, 0.0f,  1.0f),
        float3( 1.0f, 0.0f,  1.0f),
        float3( 1.0f, 0.0f, -1.0f),
    };

    static const uint16_t PlaneIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    PlaneMesh_s<uint32_t> PlaneBuilder::BuildPlaneMesh32(float2 Scale)
    {
        PlaneMesh_s<uint32_t> Mesh = {};
        Mesh.Positions.resize(4);
        Mesh.Indices.resize(6);
        for (uint32_t VertIt = 0; VertIt < 4; ++VertIt)
        {
            Mesh.Positions[VertIt] = PlanePositions[VertIt] * float3(Scale.x, 0.0f, Scale.y);
        }

        for (uint32_t IndIt = 0; IndIt < 6; ++IndIt)
        {
            Mesh.Indices[IndIt] = static_cast<uint32_t>(PlaneIndices[IndIt]);
        }

        return Mesh;
    }

    PlaneMesh_s<uint16_t> PlaneBuilder::BuildPlaneMesh16(float2 Scale)
    {
        PlaneMesh_s<uint16_t> Mesh = {};
        Mesh.Positions.resize(4);
        Mesh.Indices.resize(6);
        for (uint32_t VertIt = 0; VertIt < 4; ++VertIt)
        {
            Mesh.Positions[VertIt] = PlanePositions[VertIt] * float3(Scale.x, 0.0f, Scale.y);
        }

        for (uint32_t IndIt = 0; IndIt < 6; ++IndIt)
        {
            Mesh.Indices[IndIt] = PlaneIndices[IndIt];
        }

        return Mesh;
    }
}
