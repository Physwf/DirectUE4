#include "AnimSequence.h"
#include "BonePose.h"
#include "AnimEncoding.h"
#include "AnimationRuntime.h"

struct FRetargetTracking
{
	const FCompactPoseBoneIndex PoseBoneIndex;
	const int32 SkeletonBoneIndex;

	FRetargetTracking(const FCompactPoseBoneIndex InPoseBoneIndex, const int32 InSkeletonBoneIndex)
		: PoseBoneIndex(InPoseBoneIndex), SkeletonBoneIndex(InSkeletonBoneIndex)
	{
	}
};

struct FGetBonePoseScratchArea 
{
	BoneTrackArray RotationScalePairs;
	BoneTrackArray TranslationPairs;
	BoneTrackArray AnimScaleRetargetingPairs;
	BoneTrackArray AnimRelativeRetargetingPairs;
	BoneTrackArray OrientAndScaleRetargetingPairs;
	std::vector<FRetargetTracking> RetargetTracking;
	std::vector<FVirtualBoneCompactPoseData> VirtualBoneCompactPoseData;

	static FGetBonePoseScratchArea& Get()
	{
		static FGetBonePoseScratchArea Instance;
		return Instance;
	}
};

void UAnimSequence::CleanAnimSequenceForImport()
{
	RawAnimationData.clear();
	//RawDataGuid.Invalidate();
	AnimationTrackNames.clear();
	TrackToSkeletonMapTable.clear();
	//CompressedTrackOffsets.Empty(0);
	//CompressedByteStream.Empty(0);
	//CompressedScaleOffsets.Empty(0);
	SourceRawAnimationData.clear();
}

void UAnimSequence::GetAnimationPose(FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const
{
	if (UseRawDataForPoseExtraction(OutPose.GetBoneContainer()) && IsValidAdditive())
	{
		if (AdditiveAnimType == AAT_LocalSpaceBase)
		{
			GetBonePose_Additive(OutPose, OutCurve, ExtractionContext);
		}
		else if (AdditiveAnimType == AAT_RotationOffsetMeshSpace)
		{
			GetBonePose_AdditiveMeshRotationOnly(OutPose, OutCurve, ExtractionContext);
		}
	}
	else
	{
		GetBonePose(OutPose, OutCurve, ExtractionContext);
	}
}

void UAnimSequence::GetBonePose(FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext, bool bForceUseRawData /*= false*/) const
{
	const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();
	const bool bUseRawDataForPoseExtraction = bForceUseRawData || UseRawDataForPoseExtraction(RequiredBones);

	const bool bIsBakedAdditive = !bUseRawDataForPoseExtraction && IsValidAdditive();

	const USkeleton* MySkeleton = GetSkeleton();
	if (!MySkeleton)
	{
		if (bIsBakedAdditive)
		{
			OutPose.ResetToAdditiveIdentity();
		}
		else
		{
			OutPose.ResetToRefPose();
		}
		return;
	}

	const bool bDisableRetargeting = RequiredBones.GetDisableRetargeting();

	if (bIsBakedAdditive)
	{
		//When using baked additive ref pose is identity
		OutPose.ResetToAdditiveIdentity();
	}
	else
	{
		// if retargeting is disabled, we initialize pose with 'Retargeting Source' ref pose.
		if (bDisableRetargeting)
		{
			std::vector<FTransform> const& AuthoredOnRefSkeleton = MySkeleton->GetRefLocalPoses(RetargetSource);
			std::vector<FBoneIndexType> const& RequireBonesIndexArray = RequiredBones.GetBoneIndicesArray();

			uint32 const NumRequiredBones = RequireBonesIndexArray.size();
			for (FCompactPoseBoneIndex PoseBoneIndex : OutPose.ForEachBoneIndex())
			{
				int32 const& SkeletonBoneIndex = RequiredBones.GetSkeletonIndex(PoseBoneIndex);

				// Pose bone index should always exist in Skeleton
				assert(SkeletonBoneIndex != INDEX_NONE);
				OutPose[PoseBoneIndex] = AuthoredOnRefSkeleton[SkeletonBoneIndex];
			}
		}
		else
		{
			OutPose.ResetToRefPose();
		}
	}

	// extract curve data . Even if no track, it can contain curve data
	EvaluateCurveData(OutCurve, ExtractionContext.CurrentTime, bUseRawDataForPoseExtraction);

	const uint32 NumTracks = bUseRawDataForPoseExtraction ? TrackToSkeletonMapTable.size() : CompressedTrackToSkeletonMapTable.size();
	if (NumTracks == 0)
	{
		return;
	}

	std::vector<int32> const& SkeletonToPoseBoneIndexArray = RequiredBones.GetSkeletonToPoseBoneIndexArray();

	BoneTrackArray& RotationScalePairs = FGetBonePoseScratchArea::Get().RotationScalePairs;
	BoneTrackArray& TranslationPairs = FGetBonePoseScratchArea::Get().TranslationPairs;
	BoneTrackArray& AnimScaleRetargetingPairs = FGetBonePoseScratchArea::Get().AnimScaleRetargetingPairs;
	BoneTrackArray& AnimRelativeRetargetingPairs = FGetBonePoseScratchArea::Get().AnimRelativeRetargetingPairs;
	BoneTrackArray& OrientAndScaleRetargetingPairs = FGetBonePoseScratchArea::Get().OrientAndScaleRetargetingPairs;

	// build a list of desired bones
	RotationScalePairs.clear();
	TranslationPairs.clear();
	AnimScaleRetargetingPairs.clear();
	AnimRelativeRetargetingPairs.clear();
	OrientAndScaleRetargetingPairs.clear();

	assert((SkeletonToPoseBoneIndexArray[0] == 0));

	const bool bFirstTrackIsRootBone = (GetSkeletonIndexFromCompressedDataTrackIndex(0) == 0);

	{
		for (uint32 TrackIndex = (bFirstTrackIsRootBone ? 1 : 0); TrackIndex < NumTracks; TrackIndex++)
		{
			const int32 SkeletonBoneIndex = GetSkeletonIndexFromCompressedDataTrackIndex(TrackIndex);
			// not sure it's safe to assume that SkeletonBoneIndex can never be INDEX_NONE
			if (SkeletonBoneIndex != INDEX_NONE)
			{
				const FCompactPoseBoneIndex BoneIndex = RequiredBones.GetCompactPoseIndexFromSkeletonIndex(SkeletonBoneIndex);
				//Nasty, we break our type safety, code in the lower levels should be adjusted for this
				const int32 CompactPoseBoneIndex = BoneIndex.GetInt();
				if (CompactPoseBoneIndex != INDEX_NONE)
				{
					RotationScalePairs.push_back(BoneTrackPair(CompactPoseBoneIndex, TrackIndex));

					// Skip extracting translation component for EBoneTranslationRetargetingMode::Skeleton.
					switch (MySkeleton->GetBoneTranslationRetargetingMode(SkeletonBoneIndex))
					{
					case EBoneTranslationRetargetingMode::Animation:
						TranslationPairs.push_back(BoneTrackPair(CompactPoseBoneIndex, TrackIndex));
						break;
					case EBoneTranslationRetargetingMode::AnimationScaled:
						TranslationPairs.push_back(BoneTrackPair(CompactPoseBoneIndex, TrackIndex));
						AnimScaleRetargetingPairs.push_back(BoneTrackPair(CompactPoseBoneIndex, SkeletonBoneIndex));
						break;
					case EBoneTranslationRetargetingMode::AnimationRelative:
						TranslationPairs.push_back(BoneTrackPair(CompactPoseBoneIndex, TrackIndex));

						// With baked additives, we can skip 'AnimationRelative' tracks, as the relative transform gets canceled out.
						// (A1 + Rel) - (A2 + Rel) = A1 - A2.
						if (!bIsBakedAdditive)
						{
							AnimRelativeRetargetingPairs.push_back(BoneTrackPair(CompactPoseBoneIndex, SkeletonBoneIndex));
						}
						break;
					case EBoneTranslationRetargetingMode::OrientAndScale:
						TranslationPairs.push_back(BoneTrackPair(CompactPoseBoneIndex, TrackIndex));

						// Additives remain additives, they're not retargeted.
						if (!bIsBakedAdditive)
						{
							OrientAndScaleRetargetingPairs.push_back(BoneTrackPair(CompactPoseBoneIndex, SkeletonBoneIndex));
						}
						break;
					}
				}
			}
		}
	}

	{
		// Handle Root Bone separately
		if (bFirstTrackIsRootBone)
		{
			const int32 TrackIndex = 0;
			FCompactPoseBoneIndex RootBone(0);
			FTransform& RootAtom = OutPose[RootBone];

			AnimationFormat_GetBoneAtom(
				RootAtom,
				*this,
				TrackIndex,
				ExtractionContext.CurrentTime);

			// @laurent - we should look into splitting rotation and translation tracks, so we don't have to process translation twice.
			RetargetBoneTransform(RootAtom, 0, RootBone, RequiredBones, bIsBakedAdditive);
		}

		if (RotationScalePairs.size() > 0)
		{
			// get the remaining bone atoms
			OutPose.PopulateFromAnimation( //@TODO:@ANIMATION: Nasty hack, be good to not have this function on the pose
				*this,
				RotationScalePairs,
				TranslationPairs,
				RotationScalePairs,
				ExtractionContext.CurrentTime);

			/*AnimationFormat_GetAnimationPose(
			*((FTransformArray*)&OutAtoms),
			RotationScalePairs,
			TranslationPairs,
			RotationScalePairs,
			*this,
			ExtractionContext.CurrentTime);*/
		}
	}
}

void UAnimSequence::GetBonePose_Additive(FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const
{

}

bool UAnimSequence::IsValidAdditive() const
{
	if (AdditiveAnimType != AAT_None)
	{
		switch (RefPoseType)
		{
		case ABPT_RefPose:
			return true;
		case ABPT_AnimScaled:
			return (RefPoseSeq != NULL);
		case ABPT_AnimFrame:
			return (RefPoseSeq != NULL) && (RefFrameIndex >= 0);
		default:
			return false;
		}
	}

	return false;
}

void UAnimSequence::EvaluateCurveData(FBlendedCurve& OutCurve, float CurrentTime, bool bForceUseRawData /*= false*/) const
{

}

void UAnimSequence::GetAdditiveBasePose(FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const
{

}

void UAnimSequence::GetBonePose_AdditiveMeshRotationOnly(FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const
{

}

void UAnimSequence::RetargetBoneTransform(FTransform& BoneTransform, const int32 SkeletonBoneIndex, const FCompactPoseBoneIndex& BoneIndex, const FBoneContainer& RequiredBones, const bool bIsBakedAdditive) const
{
	const USkeleton* MySkeleton = GetSkeleton();
	FAnimationRuntime::RetargetBoneTransform(MySkeleton, RetargetSource, BoneTransform, SkeletonBoneIndex, BoneIndex, RequiredBones, bIsBakedAdditive);
}

bool UAnimSequence::UseRawDataForPoseExtraction(const FBoneContainer& RequiredBones) const
{
	//return bUseRawDataOnly || (GetSkeletonVirtualBoneGuid() != GetSkeleton()->GetVirtualBoneGuid()) || RequiredBones.GetDisableRetargeting() || RequiredBones.ShouldUseRawData() || RequiredBones.ShouldUseSourceData();
	return true;
}

