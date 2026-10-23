#include "HovercraftGymFlowComponent.h"

#include "Components/LandingPadComponent.h"
#include "Objects/HovercraftObject.h"

#include <Shared/Logging/Logging.h>
#include <Space/Space.h>
#include <Utility/SharedPtr.h>

#include <vector>

void HovercraftGymFlowComponent_c::OnCreate()
{
	Super::OnCreate();

	if (!IsActiveFlow())
		return;

	Space_c* Space = GetSpace();
	if (!Space)
		return;

	float3 PadPosition;
	float PadYaw = 0.0f;
	if (!FindSpawnPad(PadPosition, PadYaw))
		return;

	std::shared_ptr<HovercraftObject_c> NewHovercraft = Space->CreateObject<HovercraftObject_c>();
	NewHovercraft->SetFlow(SharedFrom(this));
	NewHovercraft->Respawn(PadPosition, PadYaw);

	Hovercraft = NewHovercraft;
}

void HovercraftGymFlowComponent_c::Update(float Delta)
{
	Super::Update(Delta);

	if (!RespawnPending)
		return;

	RespawnTimeRemaining -= Delta;
	if (RespawnTimeRemaining > 0.0f)
		return;

	RespawnPending = false;

	// Pads are found again in case they changed while the craft was dead. No pads leaves it dead.
	float3 PadPosition;
	float PadYaw = 0.0f;
	std::shared_ptr<HovercraftObject_c> HovercraftShared = Hovercraft.lock();
	if (HovercraftShared && FindSpawnPad(PadPosition, PadYaw))
	{
		HovercraftShared->Respawn(PadPosition, PadYaw);
	}
}

void HovercraftGymFlowComponent_c::OnHovercraftDied(HovercraftObject_c& DeadHovercraft)
{
	LOGINFO("[HovercraftGymFlow] Hovercraft died, respawning in %.1f s", RespawnDelay);

	RespawnTimeRemaining = RespawnDelay;
	RespawnPending = true;
}

bool HovercraftGymFlowComponent_c::FindSpawnPad(float3& OutPosition, float& OutYaw) const
{
	const Space_c* Space = GetSpace();
	if (!Space)
		return false;

	std::vector<LandingPadComponent_c*> Pads;
	Space->GetComponentsOfType(Pads);

	const SpatialObject_c* PadObject = Pads.empty() ? nullptr : dynamic_cast<const SpatialObject_c*>(Pads.front()->GetOwner());
	if (!ENSUREMSG(PadObject, "[HovercraftGymFlow] No landing pad on a spatial object in the Space, the hovercraft cannot spawn"))
		return false;

	OutPosition = PadObject->GetTransform().GetPosition();
	OutYaw = QuatToEuler(PadObject->GetTransform().GetRotationQuat()).y;
	return true;
}
