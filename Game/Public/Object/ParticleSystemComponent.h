#pragma once

#include "Object/SpatialObjectComponent.h"

class ParticleSystemComponent_c : public SpatialObjectComponent_c
{
	OBJECTCOMPONENT_BODY(ParticleSystemComponent_c, SpatialObjectComponent_c)
public:

	// Begin ObjectComponent_c interface
	virtual void OnCreate() override;
	virtual void Deserialize(const struct JsonValue_s& Data) override;
	virtual void PreDestroy() override;
	// End ObjectComponent_c interface

protected:

	struct ParticleSystemInfo_s* ParticleSystemInfo = nullptr;
};