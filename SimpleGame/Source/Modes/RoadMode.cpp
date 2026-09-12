#include "RoadMode.h"

#include <Input/Input.h>
#include <Object/CameraComponent.h>
#include <Physics/IPhysical.h>
#include <Space/Space.h>

void RoadMode_c::Enter()
{
	ColonyMode_c::Enter();


}

void RoadMode_c::UpdateMode(float Delta)
{
	ColonyMode_c::UpdateMode(Delta);


}

void RoadMode_c::ImGuiUpdate()
{
	ColonyMode_c::ImGuiUpdate();
}

void RoadMode_c::Exit()
{
	ColonyMode_c::Exit();
}

void RoadMode_c::OnClick()
{
	if (!Space)
		return;

	if (const CameraComponent_c* Camera = Space->GetCamera())
	{
		float3 Start, End;
		Camera->CalculateRayForScreenPosition(Input::GetMousePosition(), Start, End);

		IntersectionCtx_s Trace = IntersectionCtx_s::CreateLineSegmentTrace(Start, End, true);
		Space->Trace(Trace);

		if (Trace.HasHit())
		{
			// Do nothing for now.
		}
	}
}
