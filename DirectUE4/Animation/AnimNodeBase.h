#pragma once

#include "AnimTypes.h"
#include "AnimCurveTypes.h"
#include "BonePose.h"

class UAnimInstance;
struct FAnimInstanceProxy;
struct FAnimNode_Base;

struct FAnimationBaseContext
{
	FAnimInstanceProxy* AnimInstanceProxy;
protected:
	FAnimationBaseContext(UAnimInstance* InAnimInstance);
	FAnimationBaseContext(FAnimInstanceProxy* InAnimInstanceProxy);
public:
	FAnimationBaseContext(const FAnimationBaseContext& InContext);
};

struct FAnimationCacheBonesContext : public FAnimationBaseContext
{
public:
	FAnimationCacheBonesContext(FAnimInstanceProxy* InAnimInstanceProxy)
		: FAnimationBaseContext(InAnimInstanceProxy)
	{
	}
};

struct FAnimationInitializeContext : public FAnimationBaseContext
{
public:
	FAnimationInitializeContext(FAnimInstanceProxy* InAnimInstanceProxy)
		: FAnimationBaseContext(InAnimInstanceProxy)
	{
	}
};

/** Update context passed around during animation tree update */
struct FAnimationUpdateContext : public FAnimationBaseContext
{
private:
	float CurrentWeight;
	float RootMotionWeightModifier;

	float DeltaTime;
public:
	FAnimationUpdateContext(FAnimInstanceProxy* InAnimInstanceProxy, float InDeltaTime)
		: FAnimationBaseContext(InAnimInstanceProxy)
		, CurrentWeight(1.0f)
		, RootMotionWeightModifier(1.f)
		, DeltaTime(InDeltaTime)
	{
	}

	FAnimationUpdateContext FractionalWeight(float Multiplier) const
	{
		FAnimationUpdateContext Result(AnimInstanceProxy, DeltaTime);
		Result.CurrentWeight = CurrentWeight * Multiplier;
		Result.RootMotionWeightModifier = RootMotionWeightModifier;
		return Result;
	}

	FAnimationUpdateContext FractionalWeightAndRootMotion(float WeightMultiplier, float RootMotionMultiplier) const
	{
		FAnimationUpdateContext Result(AnimInstanceProxy, DeltaTime);
		Result.CurrentWeight = CurrentWeight * WeightMultiplier;
		Result.RootMotionWeightModifier = RootMotionWeightModifier * RootMotionMultiplier;

		return Result;
	}

	FAnimationUpdateContext FractionalWeightAndTime(float WeightMultiplier, float TimeMultiplier) const
	{
		FAnimationUpdateContext Result(AnimInstanceProxy, DeltaTime * TimeMultiplier);
		Result.CurrentWeight = CurrentWeight * WeightMultiplier;
		Result.RootMotionWeightModifier = RootMotionWeightModifier;
		return Result;
	}

	FAnimationUpdateContext FractionalWeightTimeAndRootMotion(float WeightMultiplier, float TimeMultiplier, float RootMotionMultiplier) const
	{
		FAnimationUpdateContext Result(AnimInstanceProxy, DeltaTime * TimeMultiplier);
		Result.CurrentWeight = CurrentWeight * WeightMultiplier;
		Result.RootMotionWeightModifier = RootMotionWeightModifier * RootMotionMultiplier;

		return Result;
	}

	// Returns the final blend weight contribution for this stage
	float GetFinalBlendWeight() const { return CurrentWeight; }

	// Returns the weight modifier for root motion (as root motion weight wont always match blend weight)
	float GetRootMotionWeightModifier() const { return RootMotionWeightModifier; }

	// Returns the delta time for this update, in seconds
	float GetDeltaTime() const { return DeltaTime; }
};

struct FPoseContext : public FAnimationBaseContext
{
	FCompactPose	Pose;
	FBlendedCurve	Curve;

	FPoseContext(FAnimInstanceProxy* InAnimInstanceProxy, bool bInExpectsAdditivePose = false)
		: FAnimationBaseContext(InAnimInstanceProxy)
		, bExpectsAdditivePose(bInExpectsAdditivePose)
	{
		Initialize(InAnimInstanceProxy);
	}

	// This constructor allocates a new uninitialized pose, copying non-pose state from the source context
	FPoseContext(const FPoseContext& SourceContext, bool bInOverrideExpectsAdditivePose = false)
		: FAnimationBaseContext(SourceContext.AnimInstanceProxy)
		, bExpectsAdditivePose(SourceContext.bExpectsAdditivePose || bInOverrideExpectsAdditivePose)
	{
		Initialize(SourceContext.AnimInstanceProxy);
	}

	void Initialize(FAnimInstanceProxy* InAnimInstanceProxy);

	void ResetToRefPose()
	{
		if (bExpectsAdditivePose)
		{
			Pose.ResetToAdditiveIdentity();
		}
		else
		{
			Pose.ResetToRefPose();
		}
	}

	void ResetToAdditiveIdentity()
	{
		Pose.ResetToAdditiveIdentity();
	}

	bool ContainsNaN() const
	{
		return Pose.ContainsNaN();
	}

	bool IsNormalized() const
	{
		return Pose.IsNormalized();
	}

	FPoseContext& operator=(const FPoseContext& Other)
	{
		if (AnimInstanceProxy != Other.AnimInstanceProxy)
		{
			Initialize(AnimInstanceProxy);
		}

		Pose = Other.Pose;
		Curve = Other.Curve;
		bExpectsAdditivePose = Other.bExpectsAdditivePose;
		return *this;
	}

	// Is this pose expected to be additive
	bool ExpectsAdditivePose() const { return bExpectsAdditivePose; }

private:
	bool bExpectsAdditivePose;
};
struct FAnimNode_Base
{
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context);
	virtual void Evaluate_AnyThread(FPoseContext& Output);
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context);
};