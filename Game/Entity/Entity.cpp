#include "Entity.h"

struct EntityRegistry_s
{
	uint32_t LastEntity = 0;

	Entity_t CreateEntity()
	{
		LastEntity++;
		return (Entity_t)LastEntity;
	}

	void DestroyEntity(Entity_t Entity)
	{
		// Notify fragments
	}
};

EntityRegistry_s g_EntityRegistry;

Entity_t CreateEntity()
{
	return g_EntityRegistry.CreateEntity();
}

void DestroyEntity(Entity_t Entity)
{
	g_EntityRegistry.DestroyEntity(Entity);
}
