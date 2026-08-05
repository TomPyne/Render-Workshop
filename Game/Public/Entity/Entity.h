#pragma once

#include <cmath>

enum class Entity_t : uint32_t { INVALID };

Entity_t CreateEntity();
void DestroyEntity(Entity_t Entity);