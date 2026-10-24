#include "EnemyManager.h"

#include "AI/NavGrid.h"
#include "Components/DeathVolumeComponent.h"
#include "Components/NestComponent.h"
#include "Objects/EnemyObject.h"
#include "World/Ground.h"

#include <Object/SpatialObject.h>
#include <Space/Space.h>
#include <Utility/SharedPtr.h>

#include <algorithm>
#include <cmath>

namespace
{
	float LengthXZ(float3 V)
	{
		return std::sqrt(V.x * V.x + V.z * V.z);
	}

	float3 DirectionXZ(float3 From, float3 To)
	{
		const float3 Offset(To.x - From.x, 0.0f, To.z - From.z);
		const float Length = LengthXZ(Offset);
		return Length > 0.0f ? float3(Offset.x / Length, 0.0f, Offset.z / Length) : float3(0.0f);
	}
}

void EnemyManager_c::Init(Space_c& InSpace, const NavGrid_c& InGrid)
{
	Space = &InSpace;
	Grid = &InGrid;

	std::vector<NestComponent_c*> NestComponents;
	Space->GetComponentsOfType(NestComponents);
	for (NestComponent_c* Nest : NestComponents)
	{
		Nests.push_back(SharedFrom(Nest));
	}

	std::vector<DeathVolumeComponent_c*> VolumeComponents;
	Space->GetComponentsOfType(VolumeComponents);
	for (DeathVolumeComponent_c* Volume : VolumeComponents)
	{
		Volumes.push_back({ SharedFrom(Volume) });
	}

	RefreshVolumes();
}

void EnemyManager_c::Shutdown()
{
	if (Space)
	{
		for (Enemy_s& Enemy : Enemies)
		{
			Space->DestroyObject(Enemy.Object.get());
		}
	}

	Enemies.clear();
}

void EnemyManager_c::Update(float Delta)
{
	if (!Space || !Grid)
		return;

	RefreshVolumes();
	UpdateSpawning(Delta);

	for (Enemy_s& Enemy : Enemies)
	{
		Enemy.RepathCooldown = Max(Enemy.RepathCooldown - Delta, 0.0f);

		if (Enemy.Target < 0 || !Volumes[Enemy.Target].Valid)
		{
			Enemy.Target = ChooseTarget(Enemy.Position);
			RequestPath(Enemy);
		}

		const float3 Direction = GetSteerDirection(Enemy);
		Enemy.Position = MoveOnGrid(Enemy.Position, Direction * (EnemySpeed * Delta));
	}

	DeadIndices.clear();
	for (size_t Index = 0; Index < Enemies.size(); ++Index)
	{
		if (IsTouchingVolume(Enemies[Index]))
		{
			DeadIndices.push_back(Index);
		}
	}

	// Descending, so each swap brings down an enemy that has already been checked
	for (auto It = DeadIndices.rbegin(); It != DeadIndices.rend(); ++It)
	{
		Space->DestroyObject(Enemies[*It].Object.get());
		Enemies[*It] = std::move(Enemies.back());
		Enemies.pop_back();
		++DiedCount;
	}

	for (Enemy_s& Enemy : Enemies)
	{
		Enemy.Object->SetPosition(float3(Enemy.Position.x, GetGroundHeight(Enemy.Position.x, Enemy.Position.z) + EnemyRadius, Enemy.Position.z));
	}
}

void EnemyManager_c::RefreshVolumes()
{
	for (Volume_s& Volume : Volumes)
	{
		const std::shared_ptr<DeathVolumeComponent_c> Component = Volume.Component.lock();
		const SpatialObject_c* Owner = Component ? dynamic_cast<const SpatialObject_c*>(Component->GetOwner()) : nullptr;

		Volume.Valid = Owner && !Owner->IsDestroyed();
		if (Volume.Valid)
		{
			Volume.Position = Owner->GetWorldPosition();
			Volume.HalfExtents = Component->GetFootprintHalfExtents();
		}
	}
}

void EnemyManager_c::UpdateSpawning(float Delta)
{
	SpawnTimer += Delta;

	// Ticks at the cap are dropped rather than queued
	while (SpawnTimer >= SpawnInterval)
	{
		SpawnTimer -= SpawnInterval;

		for (int Count = 0; Count < SpawnsPerTick && GetLiveCount() < MaxLiveEnemies; ++Count)
		{
			SpawnEnemy();
		}
	}
}

void EnemyManager_c::SpawnEnemy()
{
	const SpatialObject_c* NestOwner = nullptr;
	for (size_t Attempt = 0; Attempt < Nests.size() && !NestOwner; ++Attempt)
	{
		const std::shared_ptr<NestComponent_c> Nest = Nests[NextNest++ % Nests.size()].lock();
		NestOwner = Nest ? dynamic_cast<const SpatialObject_c*>(Nest->GetOwner()) : nullptr;
	}

	if (!NestOwner)
		return;

	float3 Position = NestOwner->GetWorldPosition();
	if (!Grid->IsWalkable(Grid->WorldToCell(Position)) && !Grid->FindNearestWalkable(Position, Pathfinder.SnapRadius, Position))
		return;

	Enemy_s Enemy;
	Enemy.Position = float3(Position.x, 0.0f, Position.z);
	Enemy.Object = Space->CreateObject<EnemyObject_c>();
	Enemy.Object->SetScale(EnemyRadius);
	Enemy.Target = ChooseTarget(Enemy.Position);
	RequestPath(Enemy);

	Enemies.push_back(std::move(Enemy));
	++SpawnedCount;
}

int EnemyManager_c::ChooseTarget(float3 Position) const
{
	int Best = -1;
	float BestDistance = FLT_MAX;

	for (size_t Index = 0; Index < Volumes.size(); ++Index)
	{
		if (!Volumes[Index].Valid)
			continue;

		const float Distance = LengthXZ(Volumes[Index].Position - Position);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = static_cast<int>(Index);
		}
	}

	return Best;
}

void EnemyManager_c::RequestPath(Enemy_s& Enemy)
{
	Enemy.Path.clear();
	Enemy.PathCursor = 0;

	if (Enemy.Target < 0 || Enemy.RepathCooldown > 0.0f)
		return;

	if (!Pathfinder.FindPath(*Grid, Enemy.Position, Volumes[Enemy.Target].Position, Enemy.Path))
	{
		Enemy.RepathCooldown = RepathRetryDelay;
		return;
	}

	// The first waypoint is the start cell, which the enemy is already in
	Enemy.PathCursor = Enemy.Path.size() > 1 ? 1 : 0;
}

float3 EnemyManager_c::GetSteerDirection(Enemy_s& Enemy)
{
	if (Enemy.Target < 0)
		return float3(0.0f);

	const float3 TargetPosition = Volumes[Enemy.Target].Position;
	if (LengthXZ(TargetPosition - Enemy.Position) <= DirectSteerRange && Grid->HasLineOfSight(Enemy.Position, TargetPosition))
		return DirectionXZ(Enemy.Position, TargetPosition);

	while (Enemy.PathCursor < Enemy.Path.size() && LengthXZ(Enemy.Path[Enemy.PathCursor] - Enemy.Position) <= WaypointReachedDistance)
	{
		++Enemy.PathCursor;
	}

	if (Enemy.PathCursor >= Enemy.Path.size())
	{
		RequestPath(Enemy);
		if (Enemy.PathCursor >= Enemy.Path.size())
			return float3(0.0f);
	}

	return DirectionXZ(Enemy.Position, Enemy.Path[Enemy.PathCursor]);
}

float3 EnemyManager_c::MoveOnGrid(float3 From, float3 Step) const
{
	// Substeps stay under half a cell, so a long frame cannot tunnel through a blocked cell
	const float MaxSubstep = Grid->CellSize * 0.5f;
	const int Substeps = Max(1, static_cast<int>(std::ceil(LengthXZ(Step) / MaxSubstep)));
	const float3 Substep = Step / static_cast<float>(Substeps);

	float3 Position = From;
	for (int Index = 0; Index < Substeps; ++Index)
	{
		const float3 Candidates[] = {
			Position + Substep,
			Position + float3(Substep.x, 0.0f, 0.0f),
			Position + float3(0.0f, 0.0f, Substep.z),
		};

		for (const float3& Candidate : Candidates)
		{
			if (Grid->IsWalkable(Grid->WorldToCell(Candidate)))
			{
				Position = Candidate;
				break;
			}
		}
	}

	return Position;
}

bool EnemyManager_c::IsTouchingVolume(const Enemy_s& Enemy) const
{
	for (const Volume_s& Volume : Volumes)
	{
		if (!Volume.Valid)
			continue;

		const float ClosestX = Clamp(Enemy.Position.x, Volume.Position.x - Volume.HalfExtents.x, Volume.Position.x + Volume.HalfExtents.x);
		const float ClosestZ = Clamp(Enemy.Position.z, Volume.Position.z - Volume.HalfExtents.y, Volume.Position.z + Volume.HalfExtents.y);
		const float DX = Enemy.Position.x - ClosestX;
		const float DZ = Enemy.Position.z - ClosestZ;

		if (DX * DX + DZ * DZ <= EnemyRadius * EnemyRadius)
			return true;
	}

	return false;
}
