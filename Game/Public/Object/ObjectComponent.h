#pragma once

#include "Object/ObjectMacros.h"

#include <memory>

class Object_c;

struct ObjectComponentArgs_s
{
	ObjectComponentArgs_s(const std::shared_ptr<Object_c>& InOwner)
		: Owner(InOwner)
	{}
	std::weak_ptr<Object_c> Owner;
};

class ObjectComponent_c : public std::enable_shared_from_this<ObjectComponent_c>
{
public:

	// ObjectComponent_c is the root of the hierarchy, so it has no Super and declares Self by hand.
	using Self = ObjectComponent_c;

	ObjectComponent_c(const ObjectComponentArgs_s& Args);
	virtual ~ObjectComponent_c() = default;

	// Pure so that a derived class missing its OBJECTCOMPONENT_BODY stays abstract and cannot be created.
	virtual std::wstring_view ClassName() const = 0;

	virtual void OnConstruct() {}
	virtual void Deserialize(const struct JsonValue_s& Data) {}
	virtual void Load() {}
	virtual void OnCreate() {}
	virtual void Update(float Delta) {}
	virtual void PreDestroy() {}

	inline Object_c* GetOwner() const { return Owner.lock().get();	}
	
	class Space_c* GetSpace() const;

private:

	std::weak_ptr<Object_c> Owner;
};