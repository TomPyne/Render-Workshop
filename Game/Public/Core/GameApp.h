#pragma once

#include <Render/RenderTypes.h>
#include <SurfClock.h>
#include <SurfMath.h>
#include <windef.h>

#include <memory>

extern class GameApp_c* GApp;

class GameApp_c
{
public:
	virtual ~GameApp_c() = default;

	virtual bool Init();
	virtual void Main();
	virtual void Shutdown();

	virtual std::shared_ptr<class SpaceRenderer_c> CreateSpaceRenderer() const;
	virtual void RegisterClasses();
	virtual void RegisterMaterials();

	virtual void Load(); // Called after init and before first frame

	virtual void PreUpdate(); // Init frame
	virtual void Update(float Delta);
	virtual void Render();

	virtual void ImGuiUpdate();

	virtual void Resize(int Width, int Height);
	virtual LRESULT HandleWindowsMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	virtual uint2 GetScreenSize() const;

protected:
	virtual rl::RenderInitParams GetAppRenderParams() const;

	std::shared_ptr<rl::RenderView> MainRenderView;

	std::shared_ptr<class Space_c> Space;
	std::shared_ptr<class SpaceRenderer_c> SpaceRenderer;

	SurfClock Clock;
};