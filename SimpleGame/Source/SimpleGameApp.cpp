#include "SimpleGameApp.h"

#include "Components/FighterControllerComponent.h"
#include "Levels/SimpleLevel.h"

#include <Shared/FileUtils/PathUtils.h>

void SimpleGameApp_c::RegisterClasses()
{
	GameApp_c::RegisterClasses();

	if (!Space)
		return;

	Space->RegisterComponentClass<FighterControllerComponent_c>(L"FighterControllerComponent");
}

void SimpleGameApp_c::Load()
{
	GameApp_c::Load();

	Path_s Path = Path_s(PathDirectory_e::Assets, L"Levels/FlightTest.hp_lvl");

	if (Space)
	{
		Space->LoadLevel(Path);
	}
}
