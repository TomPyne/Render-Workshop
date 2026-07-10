#pragma once

#include <Game/Level/Level.h>

class SimpleLevel_c : public Level_c
{
public:
	virtual ~SimpleLevel_c() = default;

	virtual void Load() override;
};