#pragma once

#include "AnimTypes.h"
#include "AnimationAsset.h"
#include "AnimCurveTypes.h"

enum ETypeAdvanceAnim
{
	ETAA_Default,
	ETAA_Finished,
	ETAA_Looped
};

class  UAnimSequenceBase : public UAnimationAsset
{
public:
	UAnimSequenceBase();

	float SequenceLength;

	float RateScale;

	virtual void GetAnimationPose(struct FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const = 0;

	virtual int32 GetNumberOfFrames() const;

	virtual int32 GetFrameAtTime(const float Time) const;

	virtual float GetTimeAtFrame(const int32 Frame) const;

	virtual void TickAssetPlayer(FAnimTickRecord& Instance, struct FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const override;

	void TickByMarkerAsLeader(FMarkerTickRecord& Instance, FMarkerTickContext& MarkerContext, float& CurrentTime, float& OutPreviousTime, const float MoveDelta, const bool bLooping) const;
};