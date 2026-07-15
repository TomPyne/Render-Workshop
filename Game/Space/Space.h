#pragma once

#include "Game/Object/Object.h"

#include <SurfMath.h>

#include <memory>
#include <vector>

struct SpaceView_s
{
	float2 ScreenSize;
	float NearZ;
	float FarZ;
	float Fov;
	float3 Position;
	float3 LookDir;
	matrix ProjectionMatrix;
	matrix ViewMatrix;
};

class CameraComponent_c;
class Level_c;

class Space_c : public std::enable_shared_from_this<Space_c>
{
public:

	std::vector<std::shared_ptr<Object_c>> Objects;
	std::vector<std::shared_ptr<Level_c>> Levels;

	void Update(float Delta);

	template<class ObjectType>
	std::shared_ptr<ObjectType> CreateObject()
	{
		std::shared_ptr<ObjectType> NewObject = std::make_shared<ObjectType>(ObjectArgs_s{this});
		Objects.push_back(NewObject);
		NewObject->OnCreate();
		return NewObject;
	}

	// TODO: Defer destruction until end of frame.
	void DestroyObject(Object_c* Object);

	template<class LevelType>
	Level_c* LoadLevel()
	{
		std::shared_ptr<LevelType> NewLevel = std::make_shared<LevelType>(shared_from_this());
		Levels.push_back(NewLevel);
		LoadLevelInternal(NewLevel.get());
		return NewLevel.get();
	}

	void UnloadLevel(Level_c* InLevel);

	// Camera

	SpaceView_s PrimaryView;
	
	std::weak_ptr<CameraComponent_c> PrimaryCamera;

	void RegisterCameraComponent(CameraComponent_c* Camera);
	void UnregisterCameraComponent(CameraComponent_c* Camera);

protected:

	void LoadLevelInternal(Level_c* InLevel);
};