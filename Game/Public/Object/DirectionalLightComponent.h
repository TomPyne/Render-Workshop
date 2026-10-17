#pragma once

#include "Object/SpatialObjectComponent.h"
#include "Rendering/DirectionalLightInfo.h"

// The light travels along the component's world forward axis
class DirectionalLightComponent_c : public SpatialObjectComponent_c
{
	OBJECTCOMPONENT_BODY(DirectionalLightComponent_c, SpatialObjectComponent_c)
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

	void UpdateDirection();

	DirectionalLightInfo_s DirectionalLightInfo;
	bool Registered = false;
};
