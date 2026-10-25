#pragma once

ByteAddressBuffer t_DrawConstants : register(t0, space0);

#define DRAW_CONSTANTS(Type, Name) static const Type Name = t_DrawConstants.Load<Type>(0)
