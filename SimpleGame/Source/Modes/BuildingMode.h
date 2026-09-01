#pragma once

#include "ColonyMode.h"

class BuildingMode_c : public ColonyMode_c
{
public:
	using ColonyMode_c::ColonyMode_c;

	virtual void Enter() override;
	virtual void UpdateMode(float Delta) override;
	virtual void ImGuiUpdate() override;
	virtual void Exit() override;

protected:

	class SelectionCursorObject_c* SelectionCursor = nullptr;
};