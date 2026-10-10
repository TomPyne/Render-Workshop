#pragma once

#include "ObjectComponent.h"

class SkylightComponent : public ObjectComponent_c
{
	OBJECTCOMPONENT_BODY(SkylightComponent, ObjectComponent_c)
public:

	// Begin ObjectComponent_c interface
	virtual void OnCreate() override;
	virtual void Deserialize(const struct JsonValue_s& Data) override;
	virtual void PreDestroy() override;
	// End ObjectComponent_c interface

	void SetEnvironmentMap(const std::shared_ptr<struct Texture_s>& Texture);

protected:

	std::shared_ptr<struct Texture_s> EnvironmentMap;
};