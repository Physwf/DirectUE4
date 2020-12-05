#pragma once

#include "UnrealMath.h"

#include <type_traits>

template <typename T>
inline typename std::enable_if<std::is_same<T, uint32>::value, T>::type ReverseBits(T Bits)
{
	Bits = (Bits << 16) | (Bits >> 16);
	Bits = ((Bits & 0x00ff00ff) << 8) | ((Bits & 0xff00ff00) >> 8);
	Bits = ((Bits & 0x0f0f0f0f) << 4) | ((Bits & 0xf0f0f0f0) >> 4);
	Bits = ((Bits & 0x33333333) << 2) | ((Bits & 0xcccccccc) >> 2);
	Bits = ((Bits & 0x55555555) << 1) | ((Bits & 0xaaaaaaaa) >> 1);
	return Bits;
}