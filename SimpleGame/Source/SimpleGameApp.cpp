#include "SimpleGameApp.h"

#include "Levels/SimpleLevel.h"

#include <Shared/FileUtils/PathUtils.h>

void SimpleGameApp_c::Load()
{
	GameApp_c::Load();

	Path_s Path = Path_s(PathDirectory_e::Assets, L"Levels/SimpleLevel.json");

	if (Space)
	{
		Space->LoadLevel(Path);
	}
}
