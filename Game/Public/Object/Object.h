#pragma once

#include "Object/ObjectComponent.h"
#include "Object/ObjectMacros.h"

#include <memory>
#include <string>
#include <vector>

class Space_c;
struct JsonValue_s;

struct ObjectArgs_s
{
	Space_c* OwningSpace;
};

class Object_c : public std::enable_shared_from_this<Object_c>
{	
public:

	// Object_c is the root of the hierarchy, so it has no Super and declares Self by hand.
	using Self = Object_c;

	Object_c(const ObjectArgs_s& Args);

	virtual ~Object_c() = default;

	// Pure so that a derived class missing its OBJECT_BODY stays abstract and cannot be created.
	virtual std::wstring_view GetClassName() const = 0;

	// Set up any class defaults before serialization
	virtual void OnConstruct() {}

	// Parse serialized data into object
	virtual void Deserialize(const JsonValue_s& Data);

	// Called after deserialization
	virtual void OnCreate() {}

	std::vector<std::shared_ptr<class ObjectComponent_c>> Components;

	void Update(float Delta);

	// Constructs and adds component to object. If Deferred is true, OnCreate must be called manually after params are applied.
	template<class ComponentType>
	ComponentType* AddComponent(bool Deferred = false)
	{
		static_assert(std::is_same_v<ComponentType, typename ComponentType::Self>, "Component class is missing an OBJECTCOMPONENT_BODY declaration");

		const ObjectComponentArgs_s Args(shared_from_this());

		std::shared_ptr<ComponentType> NewComponent = std::make_shared<ComponentType>(Args);
		Components.push_back(NewComponent);

		NewComponent->OnConstruct();

		if (!Deferred)
		{
			NewComponent->OnCreate();
		}
		
		return NewComponent.get();
	}

	void AddComponentByName(const std::wstring& ClassName, const JsonValue_s* const Data = nullptr);

	template<typename Func>
	void ForEachComponent(Func&& Function)
	{
		for (const auto& CompPtr : Components)
		{
			if (Function(CompPtr.get()) == false)
				return;
		}
	}

	template<class ComponentType, class Func>
	void ForEachComponentType(Func&& Function)
	{
		for (const auto& CompPtr : Components)
		{
			if (auto CastedPtr = std::dynamic_pointer_cast<ComponentType>(CompPtr))
			{
				if (Function(CastedPtr.get()) == false)
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

private:

	std::weak_ptr<Space_c> OwningSpace;
};