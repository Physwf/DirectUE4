#pragma once

#include "UnrealMath.h"

typedef uint16 FBoneIndexType;

struct FBoneIndexBase
{
	FBoneIndexBase() : BoneIndex(INDEX_NONE) {}

	inline int32 GetInt() const { return BoneIndex; }

	inline bool IsRootBone() const { return BoneIndex == 0; }

protected:
	int32 BoneIndex;
};

inline int32 GetIntFromComp(const int32 InComp)
{
	return InComp;
}

inline int32 GetIntFromComp(const FBoneIndexBase& InComp)
{
	return InComp.GetInt();
}

template<class RealBoneIndexType>
struct FBoneIndexWithOperators : public FBoneIndexBase
{
	// BoneIndexType
	inline bool operator==(const RealBoneIndexType& Rhs) const
	{
		return BoneIndex == GetIntFromComp(Rhs);
	}

	inline bool operator!=(const RealBoneIndexType& Rhs) const
	{
		return BoneIndex != GetIntFromComp(Rhs);
	}

	inline bool operator>(const RealBoneIndexType& Rhs) const
	{
		return BoneIndex > GetIntFromComp(Rhs);
	}

	inline bool operator>=(const RealBoneIndexType& Rhs) const
	{
		return BoneIndex >= GetIntFromComp(Rhs);
	}

	inline bool operator<(const RealBoneIndexType& Rhs) const
	{
		return BoneIndex < GetIntFromComp(Rhs);
	}

	inline bool operator<=(const RealBoneIndexType& Rhs) const
	{
		return BoneIndex <= GetIntFromComp(Rhs);
	}

	// FBoneIndexType
	inline bool operator==(const int32 Rhs) const
	{
		return BoneIndex == GetIntFromComp(Rhs);
	}

	inline bool operator!=(const int32 Rhs) const
	{
		return BoneIndex != GetIntFromComp(Rhs);
	}

	inline bool operator>(const int32 Rhs) const
	{
		return BoneIndex > GetIntFromComp(Rhs);
	}

	inline bool operator>=(const int32 Rhs) const
	{
		return BoneIndex >= GetIntFromComp(Rhs);
	}

	inline bool operator<(const int32 Rhs) const
	{
		return BoneIndex < GetIntFromComp(Rhs);
	}

	inline bool operator<=(const int32 Rhs) const
	{
		return BoneIndex <= GetIntFromComp(Rhs);
	}

	RealBoneIndexType& operator++()
	{
		++BoneIndex;
		return *((RealBoneIndexType*)this);
	}

	RealBoneIndexType& operator--()
	{
		--BoneIndex;
		return *((RealBoneIndexType*)this);
	}

	const RealBoneIndexType& operator=(const RealBoneIndexType& Rhs)
	{
		BoneIndex = Rhs.BoneIndex;
		return Rhs;
	}
};

struct FCompactPoseBoneIndex : public FBoneIndexWithOperators < FCompactPoseBoneIndex >
{
public:
	explicit FCompactPoseBoneIndex(int32 InBoneIndex) { BoneIndex = InBoneIndex; }
};

struct FMeshPoseBoneIndex : public FBoneIndexWithOperators < FMeshPoseBoneIndex >
{
public:
	explicit FMeshPoseBoneIndex(int32 InBoneIndex) { BoneIndex = InBoneIndex; }
};

struct FSkeletonPoseBoneIndex : public FBoneIndexWithOperators < FSkeletonPoseBoneIndex >
{
public:
	explicit FSkeletonPoseBoneIndex(int32 InBoneIndex) { BoneIndex = InBoneIndex; }
};
