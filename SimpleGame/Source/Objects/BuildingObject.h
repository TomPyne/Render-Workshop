#pragma once

#include <Object/SpatialObject.h>

class BuildingObject_c : public SpatialObject_c
{
	OBJECT_BODY(BuildingObject_c, SpatialObject_c)

	// Begin Object_c interface
	virtual void Deserialize(const struct JsonValue_s& Data) override;
	virtual void OnConstruct() override;
	// End Object_c interface

protected:

	class MeshComponent_c* MeshComponent = nullptr;
};