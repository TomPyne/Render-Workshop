#pragma once

#include "ObjectComponent.h"
#include "Rendering/SkylightInfo.h"

class SkylightComponent_c : public ObjectComponent_c
{
	OBJECTCOMPONENT_BODY(SkylightComponent_c, ObjectComponent_c)
public:

	// Begin ObjectComponent_c interface
	virtual void OnCreate() override;
	virtual void Deserialize(const struct JsonValue_s& Data) override;
	virtual void PreDestroy() override;
	// End ObjectComponent_c interface

	void SetEnvironmentMap(const std::shared_ptr<struct Texture_s>& Texture);

protected:

	SkylightInfo_s SkylightInfo;
	bool Registered = false;
};
