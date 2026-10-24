#include "DeathVolumeComponent.h"

#include <Object/SpatialObject.h>

float2 DeathVolumeComponent_c::GetFootprintHalfExtents() const
{
	const SpatialObject_c* Owner = dynamic_cast<const SpatialObject_c*>(GetOwner());
	if (!Owner)
		return float2(0.0f);

	const float3 Scale = Owner->GetTransform().GetScale();
	return float2(Scale.x, Scale.z);
}
