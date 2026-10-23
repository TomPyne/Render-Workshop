#pragma once

#include <Object/SpatialObject.h>
#include <Object/ObjectComponent.h>

class SelectionCursorComponent_c : public ObjectComponent_c
{
	OBJECTCOMPONENT_BODY(SelectionCursorComponent_c, ObjectComponent_c)

	// Begin ObjectComponent_c
	virtual void OnConstruct() override;
	// End ObjectComponent_c

	void SetHit(float3 Location, float InScale);
	void UnsetHit();

protected:
	SpatialObject_c* SpatialOwner = nullptr;
	class MeshComponent_c* MeshComp[4] = { nullptr };

	float Scale = 0.1f;
};

class SelectionCursorObject_c : public SpatialObject_c
{
	OBJECT_BODY(SelectionCursorObject_c, SpatialObject_c)

	// Begin Object_c interface
	virtual void OnConstruct() override;
	// End Object_c interface

	void SetHit(float3 Location, float Scale);
	void UnsetHit();

protected:
	SelectionCursorComponent_c* CursorComp = nullptr;
};