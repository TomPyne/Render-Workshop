#pragma once

#include "Object.h"

#include "Game/utility/Transform.h"

#include <SurfMath.h>

class SpatialObject_c : public Object_c
{
public:
	SpatialObject_c(const ObjectArgs_s& Args);
	virtual ~SpatialObject_c() = default;

	template<class ComponentType, class... Args>
	ComponentType* AddSpatialComponent(Args&&... InArgs)
	{
		std::shared_ptr<ComponentType> NewComponent = std::make_shared<ComponentType>(MakeShared<SpatialObject_c>(), std::forward<Args>(InArgs)...);
		Components.push_back(NewComponent);
		NewComponent->OnCreate();
		return NewComponent.get();
	}

	const Transform_s& GetTransform() const { return Transform; }

	void SetPosition(const float3& NewPosition) { Transform.SetPosition(NewPosition); }
	void SetRotation(const float3& NewRotation) { Transform.SetRotation(NewRotation); }
	void SetScale(float NewScale) { Transform.SetScale(NewScale); }

	void Translate(const float3& Translation) { Transform.SetPosition(Transform.GetPosition() + Translation); }

protected:
	Transform_s Transform;
};