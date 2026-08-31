#pragma once

#include <Core/GameApp.h>

class SimpleGameApp_c : public GameApp_c
{
public:
	virtual ~SimpleGameApp_c() = default;

	// Begin GameApp_c interface
	virtual void RegisterClasses() override;
	virtual void Load() override;
	virtual void PreUpdate() override;
	// Endf GameApp_c interface

	void ToggleDebugCamera(bool Enabled);
};