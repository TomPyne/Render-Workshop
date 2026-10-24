#pragma once

#include "AI/NavPathfinder.h"

#include <SurfMath.h>

#include <memory>
#include <utility>
#include <vector>

class DeathVolumeComponent_c;
class EnemyObject_c;
class NavGrid_c;
class NestComponent_c;
class Space_c;

// Owns every enemy as a flat state array. Movement runs as one pass, then each object copies its state.
class EnemyManager_c
{
public:
	void Init(Space_c& InSpace, const NavGrid_c& InGrid);
	void Update(float Delta);
	void Shutdown();

	int GetLiveCount() const { return static_cast<int>(Enemies.size()); }
	int GetSpawnedCount() const { return SpawnedCount; }
	int GetDiedCount() const { return DiedCount; }
	int GetQueueLength() const { return QueueLength; }

	float SpawnInterval = 0.5f;
	int SpawnsPerTick = 3;
	int MaxLiveEnemies = 100;

	float EnemyRadius = 0.5f;
	float EnemySpeed = 6.0f;
	float DirectSteerRange = 10.0f;
	float WaypointReachedDistance = 0.25f;
	float RepathRetryDelay = 1.0f;

	int PathBudget = 8;
	float PathRefreshInterval = 0.5f;
	float GoalMoveThreshold = 1.0f;

	float VelocitySmoothingTime = 0.2f;
	float MovingSpeedThreshold = 0.1f;
	float MaxPredictionTime = 3.0f;

private:
	struct Enemy_s
	{
		float3 Position;
		std::vector<float3> Path;
		size_t PathCursor = 0;
		int Target = -1;
		float3 LastGoal;
		float PathAge = 0.0f;
		float RepathCooldown = 0.0f;
		bool DirectSteering = false;
		std::shared_ptr<EnemyObject_c> Object;
	};

	struct Volume_s
	{
		std::weak_ptr<DeathVolumeComponent_c> Component;
		float3 Position;
		float3 Velocity;
		float2 HalfExtents;
		bool Valid = false;
		bool HasPosition = false;
	};

	void RefreshVolumes(float Delta);
	void UpdateSpawning(float Delta);
	void SpawnEnemy();
	void UpdatePathQueue(float Delta);

	// Sets the target and its path. Path distance is the only rule for now, so other priorities only have to replace this.
	void ChooseTarget(Enemy_s& Enemy);

	// Moving volumes become a point at their predicted position, static ones keep their footprint
	NavGoal_s PredictGoal(const Enemy_s& Enemy, int VolumeIndex) const;
	bool IsMoving(const Volume_s& Volume) const;
	float GetRemainingPathLength(const Enemy_s& Enemy) const;
	float3 GetInterceptPoint(float3 From, const Volume_s& Volume) const;

	// The seam between movement and pathing, so a flow field can replace A* without touching the rest
	float3 GetSteerDirection(Enemy_s& Enemy);

	// Slides along X or Z when the full step would enter a blocked cell
	float3 MoveOnGrid(float3 From, float3 Step) const;
	int GetTouchedVolume(const Enemy_s& Enemy) const;

	Space_c* Space = nullptr;
	const NavGrid_c* Grid = nullptr;
	NavPathfinder_c Pathfinder;

	std::vector<std::weak_ptr<NestComponent_c>> Nests;
	std::vector<Volume_s> Volumes;
	std::vector<NavGoal_s> Goals;
	std::vector<int> GoalVolumes;
	std::vector<Enemy_s> Enemies;
	std::vector<size_t> DeadIndices;

	// Priority then enemy index, rebuilt each frame
	std::vector<std::pair<float, size_t>> PathQueue;
	int QueueLength = 0;

	float SpawnTimer = 0.0f;
	size_t NextNest = 0;
	int SpawnedCount = 0;
	int DiedCount = 0;
};
