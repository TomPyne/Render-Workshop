#pragma once

#include <string_view>
#include <type_traits>

#define OBJECT_WIDEN_INTERNAL(x) L##x
#define OBJECT_WIDEN(x) OBJECT_WIDEN_INTERNAL(x)

// Factory names drop the _c suffix, so serialized level data refers to "MeshObject", not "MeshObject_c".
constexpr std::wstring_view StripClassNameSuffix(std::wstring_view Name)
{
	return Name.ends_with(L"_c") ? Name.substr(0, Name.size() - 2) : Name;
}

// Shared implementation of OBJECT_BODY and OBJECTCOMPONENT_BODY.
//
// Omissions and typos are caught at compile time:
//  - 'using BaseClass::BaseClass' is only well formed if BaseClass is a direct base.
//  - The Super::Self assert fails if BaseClass is itself missing its body macro, so a class in the
//    middle of a hierarchy cannot silently skip it and leave Super pointing at its grandparent.
//  - ClassBodySelfCheck fails if ThisClass is not the enclosing class, which a copied body macro
//    line would otherwise leave unnoticed. It needs a member function body because the enclosing
//    class is still incomplete at the point the macro expands.
//  - ClassName is pure in both root classes, so a class missing its body macro stays abstract.
#define CLASS_BODY_INTERNAL(ThisClass, BaseClass, MacroName)                        \
public:                                                                             \
	using Self = ThisClass;                                                         \
	using Super = BaseClass;                                                        \
	using BaseClass::BaseClass;                                                     \
                                                                                    \
	static_assert(std::is_same_v<Super, Super::Self>,                               \
		#BaseClass " is missing a " MacroName " declaration");                      \
                                                                                    \
	static constexpr std::wstring_view StaticClassName()                            \
	{                                                                               \
		return StripClassNameSuffix(OBJECT_WIDEN(#ThisClass));                      \
	}                                                                               \
                                                                                    \
	virtual std::wstring_view ClassName() const override							\
	{                                                                               \
		return StaticClassName();                                                   \
	}                                                                               \
                                                                                    \
private:                                                                            \
	void ClassBodySelfCheck()                                                       \
	{                                                                               \
		static_assert(std::is_same_v<Self, std::remove_cvref_t<decltype(*this)>>,   \
			MacroName " first argument must be the enclosing class");               \
	}                                                                               \
                                                                                    \
public:

// Must be the first entry in every class deriving from Object_c.
#define OBJECT_BODY(ThisClass, BaseClass) CLASS_BODY_INTERNAL(ThisClass, BaseClass, "OBJECT_BODY")

// Must be the first entry in every class deriving from ObjectComponent_c. For a component with
// additional mixin bases, BaseClass is the ObjectComponent_c side of the hierarchy.
#define OBJECTCOMPONENT_BODY(ThisClass, BaseClass) CLASS_BODY_INTERNAL(ThisClass, BaseClass, "OBJECTCOMPONENT_BODY")
