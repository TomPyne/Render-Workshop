#pragma once

#include "Object/SpatialObject.h"

#include <memory>

class Level_c;

// Places another level's contents under this object, so moving it moves everything it loaded
class LevelInstanceObject_c : public SpatialObject_c
{
	OBJECT_BODY(LevelInstanceObject_c, SpatialObject_c)

	// Begin Object_c interface
	virtual void Deserialize(const JsonValue_s& Data) override;
	virtual void OnDestroy() override;
	// End Object_c interface

	Level_c* GetLevel() const { return Level.get(); }

private:

	std::shared_ptr<Level_c> Level;
};
