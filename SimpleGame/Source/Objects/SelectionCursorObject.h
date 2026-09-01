#pragma once

#include <Object/SpatialObject.h>
#include <Object/ObjectComponent.h>

class SelectionCursorComponent_c : public ObjectComponent_c
{
	OBJECT_BODY(SelectionCursorComponent_c, ObjectComponent_c)

	// Begin ObjectComponent_c
	virtual void OnConstruct() override;
	virtual void Update(float Delta) override;
	// End ObjectComponent_c

	void SetHit(float3 Location, float InScale);
	void UnsetHit();

protected:
	SpatialObject_c* SpatialOwner = nullptr;
	class MeshComponent_c* MeshComp[4] = { nullptr };

	float Scale = 1.0f;
};

class SelectionCursorObject_c : public SpatialObject_c
{
	OBJECT_BODY(SelectionCursorObject_c, SpatialObject_c)

	// Begin Object_c interface
	virtual void OnConstruct() override;
	void Update(float Delta);
	// End Object_c interface

protected:
	SelectionCursorComponent_c* CursorComp = nullptr;
};