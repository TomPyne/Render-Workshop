#pragma once

#include <Render/RenderTypes.h>

#include <map>
#include <memory>
#include <string>

struct RTDGlobals_s
{
	~RTDGlobals_s();

	std::map<std::wstring, std::shared_ptr<struct RTDModel_s>> ModelMap;
	std::map<std::wstring, std::shared_ptr<struct RTDMaterial_s>> MaterialMap;
	std::map<std::wstring, std::shared_ptr<struct RTDTexture_s>> TextureMap;
	rl::RaytracingScenePtr RaytracingScene = {};
};

extern RTDGlobals_s Glob;
extern const std::wstring s_AssetDirectory;