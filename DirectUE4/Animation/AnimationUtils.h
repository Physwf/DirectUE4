#pragma once

#include "UnrealMath.h"
#include "AnimSequence.h"

#include <vector>
#include <string>

class UAnimCompress;
struct FAnimCompressContext;

class FBoneData
{
public:
	FQuat		Orientation;
	FVector		Position;
	/** Bone name. */
	std::string		Name;
	/** Direct descendants.  Empty for end effectors. */
	std::vector<int32> Children;
	/** List of bone indices from parent up to root. */
	std::vector<int32>	BonesToRoot;
	/** List of end effectors for which this bone is an ancestor.  End effectors have only one element in this list, themselves. */
	std::vector<int32>	EndEffectors;
	/** If a Socket is attached to that bone */
	bool		bHasSocket;
	/** If matched as a Key end effector */
	bool		bKeyEndEffector;

	/**	@return		Index of parent bone; -1 for the root. */
	int32 GetParent() const
	{
		return GetDepth() ? BonesToRoot[0] : -1;
	}
	/**	@return		Distance to root; 0 for the root. */
	uint32 GetDepth() const
	{
		return BonesToRoot.size();
	}
	/** @return		true if this bone is an end effector (has no children). */
	bool IsEndEffector() const
	{
		return Children.size() == 0;
	}
};

struct AnimationErrorStats
{
	/** Average world-space translation error across all end-effectors **/
	float AverageError;
	/** The worst error encountered across all end effectors **/
	float MaxError;
	/** Time at which the worst error occurred */
	float MaxErrorTime;
	/** Bone on which the worst error occurred */
	int32 MaxErrorBone;

	AnimationErrorStats()
		: AverageError(0.f)
		, MaxError(0.f)
		, MaxErrorTime(0.f)
		, MaxErrorBone(0)
	{}
};

class FAnimationUtils
{
public:
	static void BuildSkeletonMetaData(USkeleton* Skeleton, std::vector<FBoneData>& OutBoneData);
	static void ComputeCompressionError(const UAnimSequence* AnimSeq, const std::vector<FBoneData>& BoneData, AnimationErrorStats& ErrorStats);

	static void CompressAnimSequence(UAnimSequence* AnimSeq, FAnimCompressContext& CompressContext);

	static void CompressAnimSequenceExplicit(
		UAnimSequence* AnimSeq,
		FAnimCompressContext& CompressContext,
		float MasterTolerance,
		bool bFirstRecompressUsingCurrentOrDefault,
		bool bForceBelowThreshold,
		bool bRaiseMaxErrorToExisting,
		bool bTryFixedBitwiseCompression,
		bool bTryPerTrackBitwiseCompression,
		bool bTryLinearKeyRemovalCompression,
		bool bTryIntervalKeyRemoval);

	static UAnimCompress* GetDefaultAnimationCompressionAlgorithm();
};