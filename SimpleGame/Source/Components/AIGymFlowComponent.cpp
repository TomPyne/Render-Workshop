#include "AIGymFlowComponent.h"

#include <Space/Space.h>

void AIGymFlowComponent_c::OnCreate()
{
	Super::OnCreate();

	if (!IsActiveFlow())
		return;

	const Space_c* Space = GetSpace();
	if (!Space)
		return;

	NavGrid.Build(*Space);
}
