#pragma once

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

class Space_c
{
public:

	std::vector<std::shared_ptr<class Object_c>> Objects;

	void Update(float Delta);

	template<class ObjectType>
	Object_c* CreateObject()
	{
		std::shared_ptr<ObjectType> NewObject = std::make_shared<ObjectType>();
		Objects.push_back(NewObject);
		return NewObject.get();
	}

	// TODO: Defer destruction until end of frame.
	void DestroyObject(Object_c* Object)
	{
		for (size_t i = 0; i < Objects.size(); i++)
		{
			if (Objects[i].get() == Object)
			{
				Objects.erase(Objects.begin() + i);
				return;
			}
		}
	}

	// Camera

	SpaceView_s PrimaryView;
	
	std::weak_ptr<class CameraComponent_c> PrimaryCamera;

	void RegisterCameraComponent(CameraComponent_c* Camera);
	void UnregisterCameraComponent(CameraComponent_c* Camera);
};