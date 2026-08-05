#include "Object/MeshComponent.h"

void MeshComponent_c::Deserialize(const JsonValue_s& Data)
{
	SpatialObjectComponent_c::Deserialize(Data);


}

void MeshComponent_c::Render(SpatialRenderingCollector_s& Collector)
{

}

void MeshComponent_c::SetMesh(const std::shared_ptr<struct Mesh_s>& InMesh)
{
	Mesh = InMesh;
}

void MeshObject_c::Deserialize(const JsonValue_s& Data)
{
	SpatialObject_c::Deserialize(Data);


}

void MeshObject_c::OnCreate()
{
	MeshComponent = AddComponent<MeshComponent_c>();
}
