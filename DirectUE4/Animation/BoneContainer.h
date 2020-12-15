#pragma once

#include "BoneIndices.h"
#include "UnrealTemplates.h"
#include "ReferenceSkeleton.h"

#include <vector>
#include <string>
#include <map>

class USkeletalMesh;
class USkeleton;

struct FVirtualBoneCompactPoseData
{
	/** Index of this virtual bone */
	FCompactPoseBoneIndex VBIndex;
	/** Index of source bone */
	FCompactPoseBoneIndex SourceIndex;
	/** Index of target bone */
	FCompactPoseBoneIndex TargetIndex;

	FVirtualBoneCompactPoseData(FCompactPoseBoneIndex InVBIndex, FCompactPoseBoneIndex InSourceIndex, FCompactPoseBoneIndex InTargetIndex)
		: VBIndex(InVBIndex)
		, SourceIndex(InSourceIndex)
		, TargetIndex(InTargetIndex)
	{}
};
struct FCurveEvaluationOption
{
	bool bAllowCurveEvaluation;
	const std::vector<std::string>* DisallowedList;
	int32 LODIndex;

	FCurveEvaluationOption(bool bInAllowCurveEvaluation = true, const std::vector<std::string>* InDisallowedList = nullptr, int32 InLODIndex = 0)
		: bAllowCurveEvaluation(bInAllowCurveEvaluation)
		, DisallowedList(InDisallowedList)
		, LODIndex(InLODIndex)
	{
	}
};
struct FOrientAndScaleRetargetingCachedData
{
	FQuat TranslationDeltaOrient;
	float TranslationScale;
	FVector SourceTranslation;
	FVector TargetTranslation;

	FOrientAndScaleRetargetingCachedData
	(
		const FQuat& InTranslationDeltaOrient,
		const float InTranslationScale,
		const FVector& InSourceTranslation,
		const FVector& InTargetTranslation
	)
		: TranslationDeltaOrient(InTranslationDeltaOrient)
		, TranslationScale(InTranslationScale)
		, SourceTranslation(InSourceTranslation)
		, TargetTranslation(InTargetTranslation)
	{
	}
};

/** Retargeting cached data for a specific Retarget Source */
struct FRetargetSourceCachedData
{
	/** Orient and Scale cached data. */
	std::vector<FOrientAndScaleRetargetingCachedData> OrientAndScaleData;

	/** LUT from CompactPoseIndex to OrientAndScaleIndex */
	std::vector<int32> CompactPoseIndexToOrientAndScaleIndex;
};
/**
* This is a native transient structure.
* Contains:
* - BoneIndicesArray: Array of RequiredBoneIndices for Current Asset. In increasing order. Mapping to current Array of Transforms (Pose).
* - BoneSwitchArray: Size of current Skeleton. true if Bone is contained in RequiredBones array, false otherwise.
**/
struct FBoneContainer
{
	std::vector<FBoneIndexType>	BoneIndicesArray;
	std::vector<bool>			BoneSwitchArray;

	void* Asset = NULL;
	USkeletalMesh* AssetSkeletalMesh = NULL;
	USkeleton* AssetSkeleton = NULL;

	const FReferenceSkeleton* RefSkeleton;

	std::vector<int32> SkeletonToPoseBoneIndexArray;
	std::vector<int32> PoseToSkeletonBoneIndexArray;
	std::vector<int32> CompactPoseToSkeletonIndex;
	std::vector<FCompactPoseBoneIndex> SkeletonToCompactPose;
	std::vector<FCompactPoseBoneIndex> CompactPoseParentBones;
	std::vector<FTransform>    CompactPoseRefPoseBones;
	std::vector<FVirtualBoneCompactPoseData> VirtualBoneCompactPoseData;
	std::vector<uint16> UIDToArrayIndexLUT;
	bool bDisableRetargeting;
	bool bUseRAWData;
	bool bUseSourceData;
public:
	FBoneContainer();
	FBoneContainer(const std::vector<FBoneIndexType>& InRequiredBoneIndexArray, const FCurveEvaluationOption& CurveEvalOption, void* InAsset);
	void InitializeTo(const std::vector<FBoneIndexType>& InRequiredBoneIndexArray, const FCurveEvaluationOption& CurveEvalOption, void* InAsset);

	const bool IsValid() const
	{
		return (Asset != NULL && (RefSkeleton != NULL) && (BoneIndicesArray.size() > 0));
	}
	void* GetAsset() const
	{
		return Asset;
	}
	USkeletalMesh* GetSkeletalMeshAsset() const
	{
		return AssetSkeletalMesh;
	}
	USkeleton* GetSkeletonAsset() const
	{
		return AssetSkeleton;
	}
	void SetDisableRetargeting(bool InbDisableRetargeting)
	{
		bDisableRetargeting = InbDisableRetargeting;
	}
	bool GetDisableRetargeting() const
	{
		return bDisableRetargeting;
	}
	void SetUseRAWData(bool InbUseRAWData)
	{
		bUseRAWData = InbUseRAWData;
	}
	bool ShouldUseRawData() const
	{
		return bUseRAWData;
	}
	void SetUseSourceData(bool InbUseSourceData)
	{
		bUseSourceData = InbUseSourceData;
	}
	bool ShouldUseSourceData() const
	{
		return bUseSourceData;
	}
	const std::vector<FBoneIndexType>& GetBoneIndicesArray() const
	{
		return BoneIndicesArray;
	}
	const std::vector<FVirtualBoneCompactPoseData>& GetVirtualBoneCompactPoseData() const { return VirtualBoneCompactPoseData; }
	const std::vector<bool>& GetBoneSwitchArray() const
	{
		return BoneSwitchArray;
	}
	const std::vector<FTransform>& GetRefPoseArray() const
	{
		return RefSkeleton->GetRefBonePose();
	}
	const FTransform& GetRefPoseTransform(const FCompactPoseBoneIndex& BoneIndex) const
	{
		return CompactPoseRefPoseBones[BoneIndex.GetInt()];
	}
	const std::vector<FTransform>& GetRefPoseCompactArray() const
	{
		return CompactPoseRefPoseBones;
	}
	void SetRefPoseCompactArray(const std::vector<FTransform>& InRefPoseCompactArray)
	{
		assert(InRefPoseCompactArray.size() == CompactPoseRefPoseBones.size());
		CompactPoseRefPoseBones = InRefPoseCompactArray;
	}
	const FReferenceSkeleton& GetReferenceSkeleton() const
	{
		return *RefSkeleton;
	}
	const int32 GetNumBones() const
	{
		return RefSkeleton->GetNum();
	}
	const uint32 GetCompactPoseNumBones() const
	{
		return BoneIndicesArray.size();
	}
	int32 GetPoseBoneIndexForBoneName(const std::string& BoneName) const;
	int32 GetParentBoneIndex(const int32 BoneIndex) const;
	FCompactPoseBoneIndex GetParentBoneIndex(const FCompactPoseBoneIndex& BoneIndex) const;
	int32 GetDepthBetweenBones(const int32 BoneIndex, const int32 ParentBoneIndex) const;
	bool BoneIsChildOf(const int32 BoneIndex, const int32 ParentBoneIndex) const;
	bool BoneIsChildOf(const FCompactPoseBoneIndex& BoneIndex, const FCompactPoseBoneIndex& ParentBoneIndex) const;
	std::vector<uint16> const& GetUIDToArrayLookupTable() const
	{
		return UIDToArrayIndexLUT;
	}
	bool Contains(FBoneIndexType NewIndex) const
	{
		return BoneSwitchArray[NewIndex];
	}
	std::vector<int32> const & GetSkeletonToPoseBoneIndexArray() const
	{
		return SkeletonToPoseBoneIndexArray;
	}
	std::vector<int32> const & GetPoseToSkeletonBoneIndexArray() const
	{
		return PoseToSkeletonBoneIndexArray;
	}
	int32 GetSkeletonIndex(const FCompactPoseBoneIndex& BoneIndex) const
	{
		return CompactPoseToSkeletonIndex[BoneIndex.GetInt()];
	}
	FCompactPoseBoneIndex GetCompactPoseIndexFromSkeletonIndex(const int32 SkeletonIndex) const
	{
		return SkeletonToCompactPose[SkeletonIndex];
	}
	FMeshPoseBoneIndex MakeMeshPoseIndex(const FCompactPoseBoneIndex& BoneIndex) const
	{
		return FMeshPoseBoneIndex(GetBoneIndicesArray()[BoneIndex.GetInt()]);
	}
	FCompactPoseBoneIndex MakeCompactPoseIndex(const FMeshPoseBoneIndex& BoneIndex) const
	{
		return FCompactPoseBoneIndex(IndexOfByKey(GetBoneIndicesArray(), BoneIndex.GetInt()));
	}
	void CacheRequiredAnimCurveUids(const FCurveEvaluationOption& CurveEvalOption);

	const FRetargetSourceCachedData& GetRetargetSourceCachedData(const std::string& InRetargetSource) const;
private:
	mutable std::map<std::string, FRetargetSourceCachedData> RetargetSourceCachedDataLUT;
	void Initialize(const FCurveEvaluationOption& CurveEvalOption);
	void RemapFromSkelMesh(USkeletalMesh const & SourceSkeletalMesh, USkeleton& TargetSkeleton);
	void RemapFromSkeleton(USkeleton const & SourceSkeleton);
};

