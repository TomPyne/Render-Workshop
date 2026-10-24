#pragma once

#include "AI/NavPathfinder.h"

#include <SurfMath.h>

#include <memory>
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

	float SpawnInterval = 0.5f;
	int SpawnsPerTick = 3;
	int MaxLiveEnemies = 100;

	float EnemyRadius = 0.5f;
	float EnemySpeed = 6.0f;
	float DirectSteerRange = 10.0f;
	float WaypointReachedDistance = 0.25f;
	float RepathRetryDelay = 1.0f;

private:
	struct Enemy_s
	{
		float3 Position;
		std::vector<float3> Path;
		size_t PathCursor = 0;
		int Target = -1;
		float RepathCooldown = 0.0f;
		std::shared_ptr<EnemyObject_c> Object;
	};

	struct Volume_s
	{
		std::weak_ptr<DeathVolumeComponent_c> Component;
		float3 Position;
		float2 HalfExtents;
		bool Valid = false;
	};

	void RefreshVolumes();
	void UpdateSpawning(float Delta);
	void SpawnEnemy();

	// Sets the target and its path. Path distance is the only rule for now, so other priorities only have to replace this.
	void ChooseTarget(Enemy_s& Enemy);

	// The seam between movement and pathing, so a flow field can replace A* without touching the rest
	float3 GetSteerDirection(Enemy_s& Enemy);

	// Slides along X or Z when the full step would enter a blocked cell
	float3 MoveOnGrid(float3 From, float3 Step) const;
	bool IsTouchingVolume(const Enemy_s& Enemy) const;

	Space_c* Space = nullptr;
	const NavGrid_c* Grid = nullptr;
	NavPathfinder_c Pathfinder;

	std::vector<std::weak_ptr<NestComponent_c>> Nests;
	std::vector<Volume_s> Volumes;
	std::vector<NavGoal_s> Goals;
	std::vector<int> GoalVolumes;
	std::vector<Enemy_s> Enemies;
	std::vector<size_t> DeadIndices;

	float SpawnTimer = 0.0f;
	size_t NextNest = 0;
	int SpawnedCount = 0;
	int DiedCount = 0;
};
