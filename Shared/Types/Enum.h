#pragma once

#include <type_traits>

#define ENUM_FLAGS(e) \
constexpr inline e operator&(e lhs, e rhs) noexcept { using U = std::underlying_type_t<e>; return static_cast<e>(static_cast<U>(lhs) & static_cast<U>(rhs)); } \
constexpr inline e operator|(e lhs, e rhs) noexcept { using U = std::underlying_type_t<e>; return static_cast<e>(static_cast<U>(lhs) | static_cast<U>(rhs)); } \
constexpr inline e& operator|=(e& lhs, e rhs) noexcept { return lhs = lhs | rhs; }

template<typename EnumT, EnumT TInvalidVal = EnumT(0)>
constexpr bool HasEnumFlags(EnumT flags, EnumT value)
{
    return (flags & value) != TInvalidVal;
}

template<typename EnumT>
constexpr EnumT AddEnumFlags(EnumT flags, EnumT value)
{
    return flags | value;
}