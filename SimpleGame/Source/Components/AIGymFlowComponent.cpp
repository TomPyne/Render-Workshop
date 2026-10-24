#include "AIGymFlowComponent.h"

#include <Space/Space.h>

void AIGymFlowComponent_c::OnCreate()
{
	Super::OnCreate();

	if (!IsActiveFlow())
		return;

	Space_c* Space = GetSpace();
	if (!Space)
		return;

	NavGrid.Build(*Space);
	EnemyManager.Init(*Space, NavGrid);
}

void AIGymFlowComponent_c::Update(float Delta)
{
	Super::Update(Delta);

	if (!IsActiveFlow())
		return;

	EnemyManager.Update(Delta);
}

void AIGymFlowComponent_c::PreDestroy()
{
	EnemyManager.Shutdown();

	Super::PreDestroy();
}
