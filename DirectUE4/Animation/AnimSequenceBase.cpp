#include "AnimSequenceBase.h"
#include "AnimationRuntime.h"

UAnimSequenceBase::UAnimSequenceBase()
	:RateScale(1.0f)
{

}

int32 UAnimSequenceBase::GetNumberOfFrames() const
{
	static float DefaultSampleRateInterval = 1.f / DEFAULT_SAMPLERATE;
	// because of float error, add small margin at the end, so it can clamp correctly
	return (SequenceLength / DefaultSampleRateInterval + KINDA_SMALL_NUMBER);
}

int32 UAnimSequenceBase::GetFrameAtTime(const float Time) const
{
	const float FrameTime = GetNumberOfFrames() > 1 ? SequenceLength / (float)(GetNumberOfFrames() - 1) : 0.0f;
	return FMath::Clamp(FMath::RoundToInt(Time / FrameTime), 0, GetNumberOfFrames() - 1);
}

float UAnimSequenceBase::GetTimeAtFrame(const int32 Frame) const
{
	const float FrameTime = GetNumberOfFrames() > 1 ? SequenceLength / (float)(GetNumberOfFrames() - 1) : 0.0f;
	return FMath::Clamp(FrameTime * Frame, 0.0f, SequenceLength);
}

void UAnimSequenceBase::TickAssetPlayer(FAnimTickRecord& Instance, struct FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const
{
	float& CurrentTime = *(Instance.TimeAccumulator);
	float PreviousTime = CurrentTime;
	const float PlayRate = Instance.PlayRateMultiplier * this->RateScale;

	float MoveDelta = 0.f;

	if (Context.IsLeader())
	{
		const float DeltaTime = Context.GetDeltaTime();
		MoveDelta = PlayRate * DeltaTime;

		Context.SetLeaderDelta(MoveDelta);
		Context.SetPreviousAnimationPositionRatio(PreviousTime / SequenceLength);

		if (MoveDelta != 0.f)
		{
			if (Instance.bCanUseMarkerSync && Context.CanUseMarkerPosition())
			{
				TickByMarkerAsLeader(*Instance.MarkerTickRecord, Context.MarkerTickContext, CurrentTime, PreviousTime, MoveDelta, Instance.bLooping);
			}
			else
			{
				// Advance time
				FAnimationRuntime::AdvanceTime(Instance.bLooping, MoveDelta, CurrentTime, SequenceLength);
				//UE_LOG(LogAnimMarkerSync, Log, TEXT("Leader (%s) (normal advance)  - PreviousTime (%0.2f), CurrentTime (%0.2f), MoveDelta (%0.2f), Looping (%d) "), *GetName(), PreviousTime, CurrentTime, MoveDelta, Instance.bLooping ? 1 : 0);
			}
		}

		Context.SetAnimationPositionRatio(CurrentTime / SequenceLength);
	}
}

void UAnimSequenceBase::TickByMarkerAsLeader(FMarkerTickRecord& Instance, FMarkerTickContext& MarkerContext, float& CurrentTime, float& OutPreviousTime, const float MoveDelta, const bool bLooping) const
{

}

