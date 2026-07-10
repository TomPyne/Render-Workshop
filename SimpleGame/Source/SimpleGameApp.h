#pragma once

#include <Game/Core/GameApp.h>

class SimpleGameApp_c : public GameApp_c
{
public:
	virtual ~SimpleGameApp_c() = default;

	// Begin GameApp_c interface
	virtual void Load() override;
	// Endf GameApp_c interface
};