#pragma once

#include "Object/SpatialObjectComponent.h"
#include "Rendering/HeightFogInfo.h"

// The fog's base height is the component's world height
class HeightFogComponent_c : public SpatialObjectComponent_c
{
	OBJECTCOMPONENT_BODY(HeightFogComponent_c, SpatialObjectComponent_c)
public:

	// Begin ObjectComponent_c interface
	virtual void OnCreate() override;
	virtual void Deserialize(const struct JsonValue_s& Data) override;
	virtual void PreDestroy() override;
	// End ObjectComponent_c interface

	// Begin SpatialObjectComponent_c interface
	virtual void OnTransformed() override;
	// End SpatialObjectComponent_c interface

protected:

	void UpdateBaseHeight();

	HeightFogInfo_s HeightFogInfo;
	bool Registered = false;
};
