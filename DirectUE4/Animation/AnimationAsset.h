#pragma once

#include "UnrealMath.h"

class USkeleton;

class UAnimationAsset
{
private:
	class USkeleton* Skeleton;

public:
	void SetSkeleton(USkeleton* NewSkeleton);
	class USkeleton* GetSkeleton() const { return Skeleton; }
};