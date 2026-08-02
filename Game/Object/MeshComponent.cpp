#include "MeshComponent.h"

void MeshComponent_c::Render(SpatialRenderingCollector_s& Collector)
{

}

void MeshComponent_c::SetMesh(const std::shared_ptr<struct Mesh_s>& InMesh)
{
	Mesh = InMesh;
}
