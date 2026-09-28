#pragma once

#include <Render/RenderTypes.h>
#include <SurfClock.h>
#include <SurfMath.h>
#include <windef.h>

#include <memory>

class AssetManager_c;

extern class GameApp_c* GApp;

class GameApp_c
{
public:
	GameApp_c();
	virtual ~GameApp_c();

	virtual bool Init();
	virtual void Main();
	virtual void Shutdown();

	virtual std::shared_ptr<class SpaceRenderer_c> CreateSpaceRenderer() const;
	virtual void RegisterClasses();

	virtual void Load(); // Called after init and before first frame

	virtual void PreUpdate(); // Init frame
	virtual void Update(float Delta);
	virtual void Render();

	virtual void ImGuiUpdate();

	// Call between ImGui::BeginMainMenuBar and ImGui::EndMainMenuBar
	void DrawViewModeMenu();

	virtual void Resize(int Width, int Height);
	virtual LRESULT HandleWindowsMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	virtual uint2 GetScreenSize() const;

	AssetManager_c* GetAssetManager() const;

protected:
	virtual rl::RenderInitParams GetAppRenderParams() const;

	std::shared_ptr<rl::RenderView> MainRenderView;

	std::shared_ptr<class Space_c> Space;
	std::shared_ptr<class SpaceRenderer_c> SpaceRenderer;

	SurfClock Clock;

	std::unique_ptr<AssetManager_c> AssetManager;
};