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
	virtual void Update(float Delta) override;
	virtual void ImGuiUpdate() override;
	// Endf GameApp_c interface

	void ToggleDebugCamera(bool Enabled);

	// Bottom of the stacks, so a level without a camera can still be flown around
	std::shared_ptr<class DebugCameraObject_c> DefaultCamera = nullptr;

	// F8 overlay on top of everything
	std::shared_ptr<class DebugCameraObject_c> DebugCamera = nullptr;
};