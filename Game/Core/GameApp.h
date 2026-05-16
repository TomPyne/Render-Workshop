#pragma once

#include <Render/RenderTypes.h>
#include <SurfClock.h>
#include <windef.h>

#include <memory>

class GameApp_c
{
public:
	virtual bool Init();
	virtual void Shutdown();
	virtual void Update();
	virtual void Render();
	virtual void Resize(int Width, int Height);
	virtual LRESULT HandleWindowsMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	virtual rl::RenderInitParams GetAppRenderParams() const;

	std::shared_ptr<rl::RenderView> MainRenderView;

	SurfClock Clock;
};