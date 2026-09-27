#pragma once

#include <Core/GameApp.h>

class SunTempleApp_c : public GameApp_c
{
public:
	virtual ~SunTempleApp_c() = default;

	// Begin GameApp_c interface
	virtual void RegisterClasses() override;
	virtual void Load() override;
	virtual void ImGuiUpdate() override;
	// End GameApp_c interface
};