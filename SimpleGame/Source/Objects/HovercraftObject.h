#pragma once

#include <Object/SpatialObject.h>

#include <memory>

class CameraComponent_c;
class GameFlowComponent_c;
class HovercraftControllerComponent_c;
class MeshComponent_c;
class SelectionCursorObject_c;

// Spawned by a game flow, not placed in levels. It owns itself, and dying only hides it until the flow respawns it.
class HovercraftObject_c : public SpatialObject_c
{
	OBJECT_BODY(HovercraftObject_c, SpatialObject_c)

	// Begin Object_c interface
	virtual void OnConstruct() override;
	virtual void OnCreate() override;
	virtual void OnDestroy() override;
	// End Object_c interface

	void SetFlow(const std::weak_ptr<GameFlowComponent_c>& InFlow) { Flow = InFlow; }

	// Hides the craft and turns its input off, the camera stays where it is
	void Die();

	// Leaves the dead state and fully resets the craft over the given ground position
	void Respawn(float3 Position, float Yaw);

	bool IsDead() const { return Dead; }

	MeshComponent_c* GetMeshComponent() const { return MeshComponent; }
	CameraComponent_c* GetCameraComponent() const { return CameraComponent; }
	SelectionCursorObject_c* GetTargetCursor() const { return TargetCursor.get(); }

protected:
	MeshComponent_c* MeshComponent = nullptr;
	CameraComponent_c* CameraComponent = nullptr;
	HovercraftControllerComponent_c* ControllerComponent = nullptr;

	std::shared_ptr<SelectionCursorObject_c> TargetCursor;

	std::weak_ptr<GameFlowComponent_c> Flow;

	bool Dead = false;
};
