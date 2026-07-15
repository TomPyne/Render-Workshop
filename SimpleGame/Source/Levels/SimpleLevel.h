#pragma once

#include <Game/Level/Level.h>

class SimpleLevel_c : public Level_c
{
public:

	using Level_c::Level_c;

	virtual void Load() override;
};