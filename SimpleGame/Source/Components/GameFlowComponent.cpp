#include "GameFlowComponent.h"

#include <Shared/Logging/Logging.h>
#include <Space/Space.h>

#include <vector>

void GameFlowComponent_c::OnCreate()
{
	Super::OnCreate();

	const Space_c* Space = GetSpace();
	if (!Space)
		return;

	std::vector<GameFlowComponent_c*> Flows;
	Space->GetComponentsOfType(Flows);

	Active = !Flows.empty() && Flows.front() == this;

	const std::wstring_view Name = ClassName();
	CLOGWARNING(!Active, "[GameFlow] %.*S is not the first game flow in the Space and will stay inactive", static_cast<int>(Name.size()), Name.data());
}
