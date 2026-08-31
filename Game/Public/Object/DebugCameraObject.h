#pragma once

#include "Object/SpatialObject.h"

class DebugCameraObject_c : public SpatialObject_c
{
	OBJECT_BODY(DebugCameraObject_c, SpatialObject_c)

	// Begin Object_c interface
	virtual void OnConstruct() override;
	// End Object_c interface
};