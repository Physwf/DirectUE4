#pragma once

#include "UnrealMath.h"

#include <vector>

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

template <typename T>
inline void AddUnique(std::vector<T>& container, const T& Value)
{
	if (std::find(container.begin(), container.end(), Value) == container.end())
	{
		container.push_back(Value);
	}
}

template <typename T>
inline bool IsValidIndex(const std::vector<T>& container, int32 Index)
{
	return Index >= 0 && int32(container.size()) > Index;
}

template <typename T>
inline bool Contains(const std::vector<T>& container, const T& Value)
{
	return std::find(container.begin(), container.end(), Value) != container.end();
}

template <typename T>
 inline int32 Find(const std::vector<T>& container, const T& Value)
{
	auto it = std::find(container.begin(), container.end(), Value);
	if (it == container.end()) return -1;
	return int32(it - container.begin());
}

template <typename T>
inline void Remove(std::vector<T>& container, const T& Value)
{
	auto It = std::find(container.begin(), container.end(), Value);
	if (It != container.end())
	{
		container.erase(It);
	}
}
template <typename ElementType, typename KeyType>
inline int32 IndexOfByKey(std::vector<ElementType> Container, const KeyType& Key)
{
	const ElementType* __restrict Start = Container.data();
	auto ArrayNum = Container.size();
	for (const ElementType* __restrict Data = Start, *__restrict DataEnd = Start + ArrayNum; Data != DataEnd; ++Data)
	{
		if (*Data == Key)
		{
			return static_cast<int32>(Data - Start);
		}
	}
	return INDEX_NONE;
}

#define STRUCT_OFFSET( struc, member )	__builtin_offsetof(struc, member)