#include "Level.h"

#include "Game/Space/Space.h"

void Level_c::Unload()
{
	if (Space_c* Space = GetSpace())
	{
		for (const std::shared_ptr<class Object_c>& LevelObject : Objects)
		{
			Space->DestroyObject(LevelObject.get());
		}
	}

	Objects.clear();
}
