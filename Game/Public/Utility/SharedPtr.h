#pragma once

#include <memory>

// Recovers a shared_ptr from a raw pointer to any std::enable_shared_from_this type.
// T is the pointer's static type, so the downcast from the enable_shared_from_this
// base cannot fail. Use SharedCast for conversions that need checking.
template<class T>
std::shared_ptr<T> SharedFrom(T* Ptr)
{
	return Ptr ? std::static_pointer_cast<T>(Ptr->shared_from_this()) : nullptr;
}

// Recovers a shared_ptr as T, returning null if Ptr is not actually a T.
template<class T, class PtrType>
std::shared_ptr<T> SharedCast(PtrType* Ptr)
{
	return Ptr ? std::dynamic_pointer_cast<T>(Ptr->shared_from_this()) : nullptr;
}
