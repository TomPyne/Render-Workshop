#include "Level/LevelInstanceObject.h"

#include "Level/Level.h"

#include <Shared/FileUtils/JsonHelpers.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

void LevelInstanceObject_c::Deserialize(const JsonValue_s& Data)
{
	Super::Deserialize(Data);

	Path_s LevelPath;
	if (!ENSUREMSG(JsonHelpers::ParsePath(Data, "Level", LevelPath), "[LevelInstanceObject] Requires a 'Level' path"))
		return;

	Space_c* Space = GetSpace();
	if (!Space)
		return;

	Level = std::make_shared<Level_c>(Space->shared_from_this());
	Level->Deserialize(LevelPath.ToWString(), this);
	Level->Load();
}

void LevelInstanceObject_c::OnDestroy()
{
	if (Level)
	{
		Level->Unload();
		Level.reset();
	}

	Super::OnDestroy();
}
