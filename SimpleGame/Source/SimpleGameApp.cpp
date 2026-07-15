#include "SimpleGameApp.h"

#include "Levels/SimpleLevel.h"

void SimpleGameApp_c::Load()
{
	GameApp_c::Load();

	if (Space)
	{
		Space->LoadLevel<SimpleLevel_c>();
	}
}
