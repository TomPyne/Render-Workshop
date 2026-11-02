#pragma once

#include "ObjectComponent.h"
#include "Rendering/SkylightInfo.h"

#include <string>

class SkylightComponent_c : public ObjectComponent_c
{
	OBJECTCOMPONENT_BODY(SkylightComponent_c, ObjectComponent_c)
public:

	// Begin ObjectComponent_c interface
	virtual void OnCreate() override;
	virtual void VisitProperties(PropertyVisitor_c& Visitor) override;
	virtual void PreDestroy() override;
	// End ObjectComponent_c interface

	void SetEnvironmentMap(const std::shared_ptr<struct Texture_s>& Texture);

protected:

	SkylightInfo_s SkylightInfo;
	std::wstring EnvironmentMapAssetPath;
	bool Registered = false;
};
