#pragma once

#include <Object/SpatialObject.h>

class SelectionCursorObject_c : public SpatialObject_c
{
	OBJECT_BODY(SelectionCursorObject_c, SpatialObject_c)

	// Begin Object_c interface
	virtual void OnConstruct() override;
	// End Object_c interface

	void SetHit(float3 Location);
	void UnsetHit();

protected:
	class MeshComponent_c* MeshComp = nullptr;
};