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

	RefreshVolumes(0.0f);
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

	RefreshVolumes(Delta);
	UpdateSpawning(Delta);
	UpdatePathQueue(Delta);

	for (Enemy_s& Enemy : Enemies)
	{
		const float3 Direction = GetSteerDirection(Enemy);
		Enemy.Position = MoveOnGrid(Enemy.Position, Direction * (EnemySpeed * Delta));
	}

	DeadIndices.clear();
	for (size_t Index = 0; Index < Enemies.size(); ++Index)
	{
		if (GetTouchedVolume(Enemies[Index]) >= 0)
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

void EnemyManager_c::RefreshVolumes(float Delta)
{
	const float Blend = Delta > 0.0f ? 1.0f - std::exp(-Delta / VelocitySmoothingTime) : 0.0f;

	for (Volume_s& Volume : Volumes)
	{
		const std::shared_ptr<DeathVolumeComponent_c> Component = Volume.Component.lock();
		const SpatialObject_c* Owner = Component ? dynamic_cast<const SpatialObject_c*>(Component->GetOwner()) : nullptr;

		Volume.Valid = Owner && !Owner->IsDestroyed();
		if (!Volume.Valid)
			continue;

		const float3 NewPosition = Owner->GetWorldPosition();
		if (Volume.HasPosition && Delta > 0.0f)
		{
			const float3 RawVelocity((NewPosition.x - Volume.Position.x) / Delta, 0.0f, (NewPosition.z - Volume.Position.z) / Delta);
			Volume.Velocity = Volume.Velocity + (RawVelocity - Volume.Velocity) * Blend;
		}

		Volume.Position = NewPosition;
		Volume.HalfExtents = Component->GetFootprintHalfExtents();
		Volume.HasPosition = true;
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

	// No target yet, so the path queue picks it up as the most urgent
	Enemy_s Enemy;
	Enemy.Position = float3(Position.x, 0.0f, Position.z);
	Enemy.Object = Space->CreateObject<EnemyObject_c>();
	Enemy.Object->SetScale(EnemyRadius);
	Enemy.Object->SetPosition(float3(Enemy.Position.x, GetGroundHeight(Enemy.Position.x, Enemy.Position.z) + EnemyRadius, Enemy.Position.z));

	Enemies.push_back(std::move(Enemy));
	++SpawnedCount;
}

void EnemyManager_c::UpdatePathQueue(float Delta)
{
	PathQueue.clear();

	for (size_t Index = 0; Index < Enemies.size(); ++Index)
	{
		Enemy_s& Enemy = Enemies[Index];
		Enemy.PathAge += Delta;
		Enemy.RepathCooldown = Max(Enemy.RepathCooldown - Delta, 0.0f);

		if (Enemy.RepathCooldown > 0.0f)
			continue;

		const bool NoTarget = Enemy.Target < 0 || !Volumes[Enemy.Target].Valid;
		const bool PathDone = Enemy.PathCursor >= Enemy.Path.size() && !Enemy.DirectSteering;
		if (NoTarget || PathDone)
		{
			PathQueue.push_back({ FLT_MAX, Index });
			continue;
		}

		if (Enemy.PathAge < PathRefreshInterval || !IsMoving(Volumes[Enemy.Target]))
			continue;

		const float3 Goal = PredictGoal(Enemy, Enemy.Target).Centre;
		if (LengthXZ(Goal - Enemy.LastGoal) > GoalMoveThreshold)
		{
			PathQueue.push_back({ Enemy.PathAge, Index });
		}
	}

	QueueLength = static_cast<int>(PathQueue.size());

	const size_t Serviced = Min(PathQueue.size(), static_cast<size_t>(Max(PathBudget, 0)));
	std::partial_sort(PathQueue.begin(), PathQueue.begin() + Serviced, PathQueue.end(),
		[](const std::pair<float, size_t>& A, const std::pair<float, size_t>& B) { return A.first > B.first; });

	for (size_t Index = 0; Index < Serviced; ++Index)
	{
		ChooseTarget(Enemies[PathQueue[Index].second]);
	}
}

void EnemyManager_c::ChooseTarget(Enemy_s& Enemy)
{
	Goals.clear();
	GoalVolumes.clear();
	for (size_t Index = 0; Index < Volumes.size(); ++Index)
	{
		if (Volumes[Index].Valid)
		{
			Goals.push_back(PredictGoal(Enemy, static_cast<int>(Index)));
			GoalVolumes.push_back(static_cast<int>(Index));
		}
	}

	Enemy.Path.clear();
	Enemy.PathCursor = 0;
	Enemy.Target = -1;
	Enemy.PathAge = 0.0f;

	int GoalIndex = -1;
	if (!Pathfinder.FindPath(*Grid, Enemy.Position, Goals, Enemy.Path, GoalIndex))
	{
		Enemy.RepathCooldown = RepathRetryDelay;
		return;
	}

	Enemy.Target = GoalVolumes[GoalIndex];
	Enemy.LastGoal = Goals[GoalIndex].Centre;

	// The first waypoint is the start cell, which the enemy is already in
	Enemy.PathCursor = Enemy.Path.size() > 1 ? 1 : 0;
}

bool EnemyManager_c::IsMoving(const Volume_s& Volume) const
{
	return LengthXZ(Volume.Velocity) > MovingSpeedThreshold;
}

NavGoal_s EnemyManager_c::PredictGoal(const Enemy_s& Enemy, int VolumeIndex) const
{
	const Volume_s& Volume = Volumes[VolumeIndex];

	// Grown by the enemy radius, as an enemy dies when its circle touches the footprint
	if (!IsMoving(Volume))
		return { Volume.Position, float2(Volume.HalfExtents.x + EnemyRadius, Volume.HalfExtents.y + EnemyRadius) };

	// Only the current target has a known path length, the others use the straight line as a lower bound
	const bool HasPath = VolumeIndex == Enemy.Target && Enemy.PathCursor < Enemy.Path.size();
	const float Distance = HasPath ? GetRemainingPathLength(Enemy) : LengthXZ(Volume.Position - Enemy.Position);
	const float Time = Min(Distance / EnemySpeed, MaxPredictionTime);

	// Off the grid or blocked falls back to the current position, which the pathfinder snaps to a walkable cell
	const float3 Predicted = Volume.Position + Volume.Velocity * Time;
	return { Grid->IsWalkable(Grid->WorldToCell(Predicted)) ? Predicted : Volume.Position };
}

float EnemyManager_c::GetRemainingPathLength(const Enemy_s& Enemy) const
{
	if (Enemy.PathCursor >= Enemy.Path.size())
		return 0.0f;

	float Length = LengthXZ(Enemy.Path[Enemy.PathCursor] - Enemy.Position);
	for (size_t Index = Enemy.PathCursor + 1; Index < Enemy.Path.size(); ++Index)
	{
		Length += LengthXZ(Enemy.Path[Index] - Enemy.Path[Index - 1]);
	}
	return Length;
}

float3 EnemyManager_c::GetInterceptPoint(float3 From, const Volume_s& Volume) const
{
	// Smallest t > 0 with |Offset + Velocity * t| = EnemySpeed * t. No solution means the volume outruns the enemy.
	const float3 Offset(Volume.Position.x - From.x, 0.0f, Volume.Position.z - From.z);
	const float3 Velocity = Volume.Velocity;

	const float A = Velocity.x * Velocity.x + Velocity.z * Velocity.z - EnemySpeed * EnemySpeed;
	const float B = 2.0f * (Offset.x * Velocity.x + Offset.z * Velocity.z);
	const float C = Offset.x * Offset.x + Offset.z * Offset.z;

	float Time = -1.0f;
	if (std::abs(A) < 1e-4f)
	{
		Time = B < 0.0f ? -C / B : -1.0f;
	}
	else
	{
		const float Discriminant = B * B - 4.0f * A * C;
		if (Discriminant >= 0.0f)
		{
			const float Root = std::sqrt(Discriminant);
			const float T0 = (-B - Root) / (2.0f * A);
			const float T1 = (-B + Root) / (2.0f * A);
			Time = T0 > 0.0f && (T1 <= 0.0f || T0 < T1) ? T0 : T1;
		}
	}

	return Time > 0.0f ? Volume.Position + Velocity * Time : Volume.Position;
}

float3 EnemyManager_c::GetSteerDirection(Enemy_s& Enemy)
{
	Enemy.DirectSteering = false;

	if (Enemy.Target < 0 || !Volumes[Enemy.Target].Valid)
		return float3(0.0f);

	const Volume_s& Volume = Volumes[Enemy.Target];
	if (LengthXZ(Volume.Position - Enemy.Position) <= DirectSteerRange && Grid->HasLineOfSight(Enemy.Position, Volume.Position))
	{
		Enemy.DirectSteering = true;
		return DirectionXZ(Enemy.Position, GetInterceptPoint(Enemy.Position, Volume));
	}

	while (Enemy.PathCursor < Enemy.Path.size() && LengthXZ(Enemy.Path[Enemy.PathCursor] - Enemy.Position) <= WaypointReachedDistance)
	{
		++Enemy.PathCursor;
	}

	// A finished path waits in the queue for its next search
	if (Enemy.PathCursor >= Enemy.Path.size())
		return float3(0.0f);

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

int EnemyManager_c::GetTouchedVolume(const Enemy_s& Enemy) const
{
	for (size_t Index = 0; Index < Volumes.size(); ++Index)
	{
		const Volume_s& Volume = Volumes[Index];
		if (!Volume.Valid)
			continue;

		const float ClosestX = Clamp(Enemy.Position.x, Volume.Position.x - Volume.HalfExtents.x, Volume.Position.x + Volume.HalfExtents.x);
		const float ClosestZ = Clamp(Enemy.Position.z, Volume.Position.z - Volume.HalfExtents.y, Volume.Position.z + Volume.HalfExtents.y);
		const float DX = Enemy.Position.x - ClosestX;
		const float DZ = Enemy.Position.z - ClosestZ;

		if (DX * DX + DZ * DZ <= EnemyRadius * EnemyRadius)
			return static_cast<int>(Index);
	}

	return -1;
}
