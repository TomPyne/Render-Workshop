#pragma once

#include "ObjectComponent.h"

#include <vector>
#include <memory>

class Space_c;

struct ObjectArgs_s
{
	Space_c* OwningSpace;
};

class Object_c : public std::enable_shared_from_this<Object_c>
{	
public:

	Object_c(const ObjectArgs_s& Args);

	virtual ~Object_c() = default;

	virtual void OnCreate() {}

	std::vector<std::shared_ptr<class ObjectComponent_c>> Components;

	void Update(float Delta);

	template<class ComponentType, class... Args>
	ComponentType* AddComponent(Args&&... InArgs)
	{
		std::shared_ptr<ComponentType> NewComponent = std::make_shared<ComponentType>(shared_from_this(), std::forward<Args>(InArgs)...);
		Components.push_back(NewComponent);
		NewComponent->OnCreate();
		return NewComponent.get();
	}

	template<typename Func>
	void ForEachComponent(Func&& Function)
	{
		for (const auto& CompPtr : Components)
		{
			if (Function(CompPtr.get()) == false)
				return;
		}
	}

	template<typename Func, class ComponentType>
	void ForEachComponentType(Func&& Function)
	{
		for (const auto& CompPtr : Components)
		{
			if (auto CastedPtr = std::dynamic_pointer_cast<ComponentType>(CompPtr))
			{
				if (Function(CompPtr.get()) == false)
					return;
			}
		}
	}

	template<class ComponentType>
	ComponentType* GetComponent()
	{
		ComponentType* Found = nullptr;
		ForEachComponentType([&Found](ComponentType* Component)
		{
			Found = Component;
			return false;
		});

		return Found;
	}

	template<class ComponentType>
	std::vector<ComponentType*> GetComponents()
	{
		std::vector<ComponentType*> Result;
		for (const auto& CompPtr : Components)
		{
			if (auto CastedPtr = std::dynamic_pointer_cast<ComponentType>(CompPtr))
			{
				Result.push_back(CastedPtr.get());
			}
		}
		return Result;
	}

	template<class ComponentType>
	void GetComponents(std::vector<ComponentType*>& OutComponents)
	{
		for (const auto& CompPtr : Components)
		{
			if (auto CastedPtr = std::dynamic_pointer_cast<ComponentType>(CompPtr))
			{
				OutComponents.push_back(CastedPtr.get());
			}
		}
	}

	void RemoveComponent(ObjectComponent_c* Component)
	{
		auto It = std::find_if(Components.begin(), Components.end(), [Component](const std::shared_ptr<ObjectComponent_c>& CompPtr)
		{
			return CompPtr.get() == Component;
		});
		if (It != Components.end())
		{
			(*It)->PreDestroy();
			Components.erase(It);
		}
	}

	Space_c* GetSpace() const
	{
		return OwningSpace.lock().get();
	}

	template<class T> 
	std::shared_ptr<T> MakeShared()
	{
		return std::dynamic_pointer_cast<T>(shared_from_this());
	}

private:

	std::weak_ptr<Space_c> OwningSpace;
};