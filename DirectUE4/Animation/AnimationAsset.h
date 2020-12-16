#pragma once

#include "UnrealMath.h"
#include "AnimEnums.h"

#include <vector>

class USkeleton;

namespace MarkerIndexSpecialValues
{
	enum Type
	{
		Unitialized = -2,
		AnimationBoundary = -1,
	};
};

struct FMarkerPair
{
	int32 MarkerIndex;
	float TimeToMarker;

	FMarkerPair() : MarkerIndex(MarkerIndexSpecialValues::Unitialized) {}

	void Reset() { MarkerIndex = MarkerIndexSpecialValues::Unitialized; }
};

struct FMarkerTickRecord
{
	//Current Position in marker space, equivalent to TimeAccumulator
	FMarkerPair PreviousMarker;
	FMarkerPair NextMarker;

	bool IsValid() const { return PreviousMarker.MarkerIndex != MarkerIndexSpecialValues::Unitialized && NextMarker.MarkerIndex != MarkerIndexSpecialValues::Unitialized; }

	void Reset() { PreviousMarker.Reset(); NextMarker.Reset(); }
};

/** Transform definition */
struct FBlendSampleData
{
	int32 SampleDataIndex;
	class UAnimSequence* Animation;
	float TotalWeight;
	float Time;
	float PreviousTime;
	float SamplePlayRate;
	FMarkerTickRecord MarkerTickRecord;
	std::vector<float> PerBoneBlendData;
	FBlendSampleData()
		: SampleDataIndex(0)
		, Animation(nullptr)
		, TotalWeight(0.f)
		, Time(0.f)
		, PreviousTime(0.f)
		, SamplePlayRate(0.0f)
	{}
	FBlendSampleData(int32 Index)
		: SampleDataIndex(Index)
		, Animation(nullptr)
		, TotalWeight(0.f)
		, Time(0.f)
		, PreviousTime(0.f)
		, SamplePlayRate(0.0f)
	{}
	bool operator==(const FBlendSampleData& Other) const
	{
		// if same position, it's same point
		return (Other.SampleDataIndex == SampleDataIndex);
	}
	void AddWeight(float Weight)
	{
		TotalWeight += Weight;
	}
	float GetWeight() const
	{
		return FMath::Clamp<float>(TotalWeight, 0.f, 1.f);
	}

	static void NormalizeDataWeight(std::vector<FBlendSampleData>& SampleDataList);
};

struct FBlendFilter
{

// 	FFIRFilterTimeBased FilterPerAxis[3];
// 
// 	FBlendFilter()
// 	{
// 	}
// 
// 	FVector GetFilterLastOutput()
// 	{
// 		return FVector(FilterPerAxis[0].LastOutput, FilterPerAxis[1].LastOutput, FilterPerAxis[2].LastOutput);
// 	}
};
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
struct FMarkerSyncAnimPosition
{
	/** The marker we have passed*/
	std::string PreviousMarkerName;
	/** The marker we are heading towards */
	std::string NextMarkerName;

	/** Value between 0 and 1 representing where we are:
	0   we are at PreviousMarker
	1   we are at NextMarker
	0.5 we are half way between the two */
	float PositionBetweenMarkers;

	/** Is this a valid Marker Sync Position */
	bool IsValid() const { return (PreviousMarkerName != "" && NextMarkerName != ""); }

	FMarkerSyncAnimPosition()
		: PositionBetweenMarkers(0.0f)
	{}

	FMarkerSyncAnimPosition(const std::string& InPrevMarkerName, const std::string& InNextMarkerName, const float& InAlpha)
		: PreviousMarkerName(InPrevMarkerName)
		, NextMarkerName(InNextMarkerName)
		, PositionBetweenMarkers(InAlpha)
	{}
};
struct FPassedMarker
{
	std::string PassedMarkerName;

	float DeltaTimeWhenPassed;
};

struct FAnimTickRecord
{
	class UAnimationAsset* SourceAsset;

	float*  TimeAccumulator;
	float PlayRateMultiplier;
	float EffectiveBlendWeight;
	float RootMotionWeightModifier;

	bool bLooping;

	union
	{
		struct
		{
			float  BlendSpacePositionX;
			float  BlendSpacePositionY;
			FBlendFilter* BlendFilter;
			std::vector<FBlendSampleData>* BlendSampleDataCache;
		} BlendSpace;

		struct
		{
			float CurrentPosition;  // montage doesn't use accumulator, but this
			float PreviousPosition;
			float MoveDelta; // MoveDelta isn't always abs(CurrentPosition-PreviousPosition) because Montage can jump or repeat or loop
			std::vector<FPassedMarker>* MarkersPassedThisTick;
		} Montage;
	};

	// marker sync related data
	FMarkerTickRecord* MarkerTickRecord;
	bool bCanUseMarkerSync;
	float LeaderScore;

	// Return the root motion weight for this tick record
	float GetRootMotionWeight() const { return EffectiveBlendWeight * RootMotionWeightModifier; }

public:
	FAnimTickRecord()
		: SourceAsset(nullptr)
		, TimeAccumulator(nullptr)
		, PlayRateMultiplier(1.f)
		, EffectiveBlendWeight(0.f)
		, RootMotionWeightModifier(1.f)
		, bLooping(false)
		, MarkerTickRecord(nullptr)
		, bCanUseMarkerSync(false)
		, LeaderScore(0.f)
	{
	}

	/** This can be used with the Sort() function on a TArray of FAnimTickRecord to sort from higher leader score */
	bool operator <(const FAnimTickRecord& Other) const { return LeaderScore > Other.LeaderScore; }
};
class FMarkerTickContext
{
public:

	static const std::vector<std::string> DefaultMarkerNames;

	FMarkerTickContext(const std::vector<std::string>& ValidMarkerNames)
		: ValidMarkers(&ValidMarkerNames)
	{}

	FMarkerTickContext()
		: ValidMarkers(&DefaultMarkerNames)
	{}

	void SetMarkerSyncStartPosition(const FMarkerSyncAnimPosition& SyncPosition)
	{
		MarkerSyncStartPostion = SyncPosition;
	}

	void SetMarkerSyncEndPosition(const FMarkerSyncAnimPosition& SyncPosition)
	{
		MarkerSyncEndPostion = SyncPosition;
	}

	const FMarkerSyncAnimPosition& GetMarkerSyncStartPosition() const
	{
		return MarkerSyncStartPostion;
	}

	const FMarkerSyncAnimPosition& GetMarkerSyncEndPosition() const
	{
		return MarkerSyncEndPostion;
	}

	const std::vector<std::string>& GetValidMarkerNames() const
	{
		return *ValidMarkers;
	}

	bool IsMarkerSyncStartValid() const
	{
		return MarkerSyncStartPostion.IsValid();
	}

	bool IsMarkerSyncEndValid() const
	{
		// does it have proper end position
		return MarkerSyncEndPostion.IsValid();
	}

	std::vector<FPassedMarker> MarkersPassedThisTick;

private:
	// Structure representing our sync position based on markers before tick
	// This is used to allow new animations to play from the right marker position
	FMarkerSyncAnimPosition MarkerSyncStartPostion;

	// Structure representing our sync position based on markers after tick
	FMarkerSyncAnimPosition MarkerSyncEndPostion;

	// Valid marker names for this sync group
	const std::vector<std::string>* ValidMarkers;
};

namespace EAnimGroupRole
{
	enum Type
	{
		/** This node can be the leader, as long as it has a higher blend weight than the previous best leader. */
		CanBeLeader,

		/** This node will always be a follower (unless there are only followers, in which case the first one ticked wins). */
		AlwaysFollower,

		/** This node will always be a leader (if more than one node is AlwaysLeader, the last one ticked wins). */
		AlwaysLeader,

		/** This node will be excluded from the sync group while blending in. Once blended in it will be the sync group leader until blended out*/
		TransitionLeader,

		/** This node will be excluded from the sync group while blending in. Once blended in it will be a follower until blended out*/
		TransitionFollower,
	};
}

struct FAnimGroupInstance
{
public:
	// The list of animation players in this group which are going to be evaluated this frame
	std::vector<FAnimTickRecord> ActivePlayers;
	// The current group leader
	// @note : before ticking, this is invalid
	// after ticking, this should contain the real leader
	// during ticket, this list gets sorted by LeaderScore of AnimTickRecord,
	// and it starts from 0 index, but if that fails due to invalid position, it will go to the next available leader
	int32 GroupLeaderIndex;
	// Valid marker names for this sync group
	std::vector<std::string> ValidMarkers;
	// Can we use sync markers for ticking this sync group
	bool bCanUseMarkerSync;

	// This has latest Montage Leader Weight
	float MontageLeaderWeight;

	FMarkerTickContext MarkerTickContext;

public:
	FAnimGroupInstance()
		: GroupLeaderIndex(INDEX_NONE)
		, bCanUseMarkerSync(false)
		, MontageLeaderWeight(0.f)
	{
	}

	void Reset()
	{
		GroupLeaderIndex = INDEX_NONE;
		ActivePlayers.clear();
		bCanUseMarkerSync = false;
		MontageLeaderWeight = 0.f;
		MarkerTickContext = FMarkerTickContext();
	}

	// Checks the last tick record in the ActivePlayers array to see if it's a better leader than the current candidate.
	// This should be called once for each record added to ActivePlayers, after the record is setup.
	void TestTickRecordForLeadership(EAnimGroupRole::Type MembershipType);
	// Checks the last tick record in the ActivePlayers array to see if we have a better leader for montage
	// This is simple rule for higher weight becomes leader
	// Only one from same sync group with highest weight will be leader
	void TestMontageTickRecordForLeadership();

	// Called after leader has been ticked and decided
	void Finalize(const FAnimGroupInstance* PreviousGroup);

	// Called after all tick records have been added but before assets are actually ticked
	void Prepare(const FAnimGroupInstance* PreviousGroup);
};
struct FRootMotionMovementParams
{

};
struct FAnimAssetTickContext
{
public:
	FAnimAssetTickContext(float InDeltaTime, ERootMotionMode::Type InRootMotionMode, bool bInOnlyOneAnimationInGroup, const std::vector<std::string>& ValidMarkerNames)
		: RootMotionMode(InRootMotionMode)
		, MarkerTickContext(ValidMarkerNames)
		, DeltaTime(InDeltaTime)
		, LeaderDelta(0.f)
		, PreviousAnimLengthRatio(0.0f)
		, AnimLengthRatio(0.0f)
		, bIsMarkerPositionValid(ValidMarkerNames.size() > 0)
		, bIsLeader(true)
		, bOnlyOneAnimationInGroup(bInOnlyOneAnimationInGroup)
	{
	}

	FAnimAssetTickContext(float InDeltaTime, ERootMotionMode::Type InRootMotionMode, bool bInOnlyOneAnimationInGroup)
		: RootMotionMode(InRootMotionMode)
		, DeltaTime(InDeltaTime)
		, LeaderDelta(0.f)
		, PreviousAnimLengthRatio(0.0f)
		, AnimLengthRatio(0.0f)
		, bIsMarkerPositionValid(false)
		, bIsLeader(true)
		, bOnlyOneAnimationInGroup(bInOnlyOneAnimationInGroup)
	{
	}

	// Are we the leader of our sync group (or ungrouped)?
	bool IsLeader() const
	{
		return bIsLeader;
	}

	bool IsFollower() const
	{
		return !bIsLeader;
	}

	// Return the delta time of the tick
	float GetDeltaTime() const
	{
		return DeltaTime;
	}

	void SetLeaderDelta(float InLeaderDelta)
	{
		LeaderDelta = InLeaderDelta;
	}

	float GetLeaderDelta() const
	{
		return LeaderDelta;
	}

	void SetPreviousAnimationPositionRatio(float NormalizedTime)
	{
		PreviousAnimLengthRatio = NormalizedTime;
	}

	void SetAnimationPositionRatio(float NormalizedTime)
	{
		AnimLengthRatio = NormalizedTime;
	}

	// Returns the previous synchronization point (normalized time; only legal to call if ticking a follower)
	float GetPreviousAnimationPositionRatio() const
	{
		assert(!bIsLeader);
		return PreviousAnimLengthRatio;
	}

	// Returns the synchronization point (normalized time; only legal to call if ticking a follower)
	float GetAnimationPositionRatio() const
	{
		assert(!bIsLeader);
		return AnimLengthRatio;
	}

	void InvalidateMarkerSync()
	{
		bIsMarkerPositionValid = false;
	}

	bool CanUseMarkerPosition() const
	{
		return bIsMarkerPositionValid;
	}

	void ConvertToFollower()
	{
		bIsLeader = false;
	}

	bool ShouldGenerateNotifies() const
	{
		return IsLeader();
	}

	bool IsSingleAnimationContext() const
	{
		return bOnlyOneAnimationInGroup;
	}

	//Root Motion accumulated from this tick context
	FRootMotionMovementParams RootMotionMovementParams;

	// The root motion mode of the owning AnimInstance
	ERootMotionMode::Type RootMotionMode;

	FMarkerTickContext MarkerTickContext;

private:
	float DeltaTime;

	float LeaderDelta;

	// Float in 0 - 1 range representing how far through an animation we were before ticking
	float PreviousAnimLengthRatio;

	// Float in 0 - 1 range representing how far through an animation we are
	float AnimLengthRatio;

	bool bIsMarkerPositionValid;

	bool bIsLeader;

	bool bOnlyOneAnimationInGroup;
};
struct FAnimNotifyQueue
{

};
class UAnimationAsset
{
private:
	class USkeleton* Skeleton;

public:
	void SetSkeleton(USkeleton* NewSkeleton);
	class USkeleton* GetSkeleton() const { return Skeleton; }

	virtual std::vector<std::string>* GetUniqueMarkerNames() { return NULL; }

	virtual void TickAssetPlayer(FAnimTickRecord& Instance, struct FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const {}

	virtual bool IsValidAdditive() const { return false; }
};