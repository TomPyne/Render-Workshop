#pragma once

#include "ColonyMode.h"

#include <SurfMath.h>

class RoadMode_c : public ColonyMode_c
{
public:
	using ColonyMode_c::ColonyMode_c;

	virtual void Enter() override;
	virtual void UpdateMode(float Delta) override;
	virtual void ImGuiUpdate() override;
	virtual void Exit() override;

protected:

	void OnClick();

	void BeginPlacingRoad(const float3& Position);
	void EndPlacingRoad(const float3& Position);
};