#include "Object/SpatialObject.h"

#include "Object/PropertyVisitor.h"
#include "Object/SpatialObjectComponent.h"
#include "Utility/SharedPtr.h"

#include <Shared/Logging/Logging.h>

SpatialObject_c::SpatialObject_c(const ObjectArgs_s& Args)
	: Object_c(Args)
{

}

void SpatialObject_c::VisitProperties(PropertyVisitor_c& Visitor)
{
	Super::VisitProperties(Visitor);

	if (VisitTransformProperties(Visitor, Transform) && !Visitor.IsLoading())
	{
		NotifyComponentsTransformed();
	}
}

void SpatialObject_c::OnDestroy()
{
	for (const std::weak_ptr<SpatialObject_c>& Child : Children)
	{
		if (std::shared_ptr<SpatialObject_c> ChildShared = Child.lock())
		{
			ChildShared->Parent.reset();
		}
	}
	Children.clear();

	SetParent(nullptr);

	Super::OnDestroy();
}

matrix SpatialObject_c::GetWorldMatrix() const
{
	const SpatialObject_c* ParentObject = GetParent();
	return ParentObject ? Transform.GetMatrix() * ParentObject->GetWorldMatrix() : Transform.GetMatrix();
}

quat SpatialObject_c::GetWorldRotation() const
{
	const SpatialObject_c* ParentObject = GetParent();
	if (!ParentObject)
	{
		return Transform.GetRotationQuat();
	}

	ENSUREMSG(ParentObject->GetTransform().IsUniformScale(), "[SpatialObject_c::GetWorldRotation] Parent has non-uniform scale, the returned rotation does not match GetWorldMatrix.");

	return Mul(Transform.GetRotationQuat(), ParentObject->GetWorldRotation());
}

void SpatialObject_c::SetParent(SpatialObject_c* NewParent)
{
	SpatialObject_c* OldParent = GetParent();
	if (NewParent == OldParent)
		return;

	for (const SpatialObject_c* Ancestor = NewParent; Ancestor; Ancestor = Ancestor->GetParent())
	{
		if (!ENSUREMSG(Ancestor != this, "[SpatialObject_c::SetParent] Parenting would create a cycle"))
			return;
	}

	if (OldParent)
	{
		std::erase_if(OldParent->Children, [this](const std::weak_ptr<SpatialObject_c>& Child)
		{
			const std::shared_ptr<SpatialObject_c> ChildShared = Child.lock();
			return !ChildShared || ChildShared.get() == this;
		});
	}

	Parent = SharedFrom(NewParent);

	if (NewParent)
	{
		NewParent->Children.push_back(SharedFrom(this));
	}

	NotifyComponentsTransformed();
}

void SpatialObject_c::SetPosition(const float3& NewPosition)
{
	Transform.SetPosition(NewPosition);
	NotifyComponentsTransformed();
}

void SpatialObject_c::SetRotation(const float3& NewRotation)
{
	Transform.SetRotation(NewRotation);
	NotifyComponentsTransformed();
}

void SpatialObject_c::SetRotation(quat NewRotation)
{
	Transform.SetRotation(NewRotation);
	NotifyComponentsTransformed();
}

void SpatialObject_c::SetScale(const float3& NewScale)
{
	Transform.SetScale(NewScale);
	NotifyComponentsTransformed();
}

void SpatialObject_c::SetScale(float NewScale)
{
	Transform.SetScale(NewScale);
	NotifyComponentsTransformed();
}

void SpatialObject_c::Translate(const float3& Translation)
{
	Transform.SetPosition(Transform.GetPosition() + Translation);
	NotifyComponentsTransformed();
}

void SpatialObject_c::Rotate(quat Delta)
{
	Transform.Rotate(Delta);
	NotifyComponentsTransformed();
}

void SpatialObject_c::RotateLocal(quat Delta)
{
	Transform.RotateLocal(Delta);
	NotifyComponentsTransformed();
}

void SpatialObject_c::ResetMotion()
{
	for (SpatialObjectComponent_c* Component : GetComponents<SpatialObjectComponent_c>())
	{
		Component->ResetMotion();
	}

	for (const std::weak_ptr<SpatialObject_c>& Child : Children)
	{
		if (std::shared_ptr<SpatialObject_c> ChildShared = Child.lock())
		{
			ChildShared->ResetMotion();
		}
	}
}

void SpatialObject_c::NotifyComponentsTransformed()
{
	for (SpatialObjectComponent_c* Component : GetComponents<SpatialObjectComponent_c>())
	{
		Component->OnTransformed();
	}

	for (const std::weak_ptr<SpatialObject_c>& Child : Children)
	{
		if (std::shared_ptr<SpatialObject_c> ChildShared = Child.lock())
		{
			ChildShared->NotifyComponentsTransformed();
		}
	}
}
