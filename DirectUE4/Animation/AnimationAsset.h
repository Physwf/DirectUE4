#pragma once

#include "UnrealMath.h"

#include <vector>

class USkeleton;

/** Animation Extraction Context */
struct FAnimExtractContext
{
	/** Is Root Motion being extracted? */
	bool bExtractRootMotion;
	/** Position in animation to extract pose from */
	float CurrentTime;
	/**
	* Pose Curve Values to extract pose from pose assets.
	* This is used by pose asset extraction
	* This always has to match with pose # in the asset it's extracting from
	*/
	std::vector<float> PoseCurves;

	FAnimExtractContext()
		: bExtractRootMotion(false)
		, CurrentTime(0.f)
	{
	}

	FAnimExtractContext(float InCurrentTime)
		: bExtractRootMotion(false)
		, CurrentTime(InCurrentTime)
	{
	}

	FAnimExtractContext(float InCurrentTime, bool InbExtractRootMotion)
		: bExtractRootMotion(InbExtractRootMotion)
		, CurrentTime(InCurrentTime)
	{
	}

	FAnimExtractContext(std::vector<float>& InPoseCurves)
		: bExtractRootMotion(false)
		, CurrentTime(0.f)
		, PoseCurves(InPoseCurves)
	{
		// @todo: no support on root motion
	}
};

class UAnimationAsset
{
private:
	class USkeleton* Skeleton;

public:
	void SetSkeleton(USkeleton* NewSkeleton);
	class USkeleton* GetSkeleton() const { return Skeleton; }

	virtual bool IsValidAdditive() const { return false; }
};