#pragma once

class Space_c;

class ColonyMode_c
{
public:

	ColonyMode_c(Space_c* InSpace)
		: Space(InSpace)
	{}

	virtual ~ColonyMode_c() = default;

	virtual void Enter() {}
	virtual void UpdateMode(float Delta) {}
	virtual void ImGuiUpdate() {}
	virtual void Exit() {}

protected:

	Space_c* Space = nullptr;
};