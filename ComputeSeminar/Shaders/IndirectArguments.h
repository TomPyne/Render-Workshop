#ifndef INDIRECT_ARGUMENTS_H
#define INDIRECT_ARGUMENTS_H

struct IndirectDrawLayout
{
    uint NumVerts;
    uint NumInstances;
    uint StartVertex;
    uint StartInstance;
};

struct IndirectDrawIndexedLayout
{
    uint NumIndices;
    uint NumInstances;
    uint StartIndex;
    uint StartVertex;
    uint StartInstance;
};

struct IndirectDispatchLayout
{
    uint X;
    uint Y;
    uint Z;
};

#endif // INDIRECT_ARGUMENTS_H