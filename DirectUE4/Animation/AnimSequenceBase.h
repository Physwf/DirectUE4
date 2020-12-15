#pragma once

#include "AnimTypes.h"
#include "AnimationAsset.h"
#include "AnimCurveTypes.h"

class  UAnimSequenceBase : public UAnimationAsset
{
public:
	float SequenceLength;

	float RateScale;

	virtual void GetAnimationPose(struct FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const = 0;
};