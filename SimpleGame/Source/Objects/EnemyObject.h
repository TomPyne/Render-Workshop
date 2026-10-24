#pragma once

#include <Object/SpatialObject.h>

class MeshComponent_c;

// Placeholder enemy body, a cube with no logic of its own
class EnemyObject_c : public SpatialObject_c
{
	OBJECT_BODY(EnemyObject_c, SpatialObject_c)

	// Begin Object_c interface
	virtual void OnConstruct() override;
	// End Object_c interface

protected:
	MeshComponent_c* MeshComponent = nullptr;
};
