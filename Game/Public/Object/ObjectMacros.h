#pragma once

#include <type_traits>

#define OBJECT_WIDEN_INTERNAL(x) L##x
#define OBJECT_WIDEN(x) OBJECT_WIDEN_INTERNAL(x)

// Declares the standard aliases, inherited constructors and class name for an object class.
// Every class deriving from Object_c must declare this, and it must be the first entry in the class.
//
// Omissions and typos are caught at compile time:
//  - 'using BaseClass::BaseClass' is only well formed if BaseClass is a direct base.
//  - The Super::Self assert fails if BaseClass is itself missing an OBJECT_BODY, so a class in the
//    middle of a hierarchy cannot silently skip it and leave Super pointing at its grandparent.
//  - ObjectBodySelfCheck fails if ThisClass is not the enclosing class, which a copied OBJECT_BODY
//    line would otherwise leave unnoticed. It needs a member function body because the enclosing
//    class is still incomplete at the point the macro expands.
#define OBJECT_BODY(ThisClass, BaseClass)                                           \
public:                                                                             \
	using Self = ThisClass;                                                         \
	using Super = BaseClass;                                                        \
	using BaseClass::BaseClass;                                                     \
                                                                                    \
	static_assert(std::is_same_v<Super, Super::Self>,                               \
		#BaseClass " is missing an OBJECT_BODY declaration");                       \
                                                                                    \
	static constexpr const wchar_t* StaticClassName()                               \
	{                                                                               \
		return OBJECT_WIDEN(#ThisClass);                                            \
	}                                                                               \
                                                                                    \
	virtual const wchar_t* GetClassName() const override                            \
	{                                                                               \
		return StaticClassName();                                                   \
	}                                                                               \
                                                                                    \
private:                                                                            \
	void ObjectBodySelfCheck()                                                      \
	{                                                                               \
		static_assert(std::is_same_v<Self, std::remove_cvref_t<decltype(*this)>>,   \
			"OBJECT_BODY first argument must be the enclosing class");              \
	}                                                                               \
                                                                                    \
public:
