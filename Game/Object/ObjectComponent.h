#pragma once

#include <memory>

class Object_c;

class ObjectComponent_c : public std::enable_shared_from_this<ObjectComponent_c>
{
public:

	ObjectComponent_c(const std::shared_ptr<Object_c>& InOwner);

	virtual void OnCreate() {}
	virtual void Update(float Delta) {}
	virtual void PreDestroy() {}

	Object_c* GetOwner() const
	{
		return Owner.lock().get();
	}
	
	class Space_c* GetSpace() const;

private:

	std::weak_ptr<Object_c> Owner;

};