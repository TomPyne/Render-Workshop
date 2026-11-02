#include "BuildingObject.h"

#include <Object/MeshComponent.h>
#include <Object/PropertyVisitor.h>

void BuildingObject_c::Deserialize(const JsonValue_s& Data)
{
	Super::Deserialize(Data);

	// Level files author the mesh fields on the object as shorthand for its component
	if (MeshComponent)
	{
		JsonReadVisitor_c Reader(Data);
		MeshComponent->VisitMeshProperties(Reader);
	}
}

void BuildingObject_c::OnConstruct()
{
	Super::OnConstruct();

	MeshComponent = AddComponent<MeshComponent_c>();
}
