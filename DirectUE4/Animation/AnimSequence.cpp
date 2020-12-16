#include "AnimSequence.h"
#include "BonePose.h"
#include "AnimEncoding.h"
#include "AnimationRuntime.h"
#include "AnimCompressionDerivedData.h"
#include "AnimCompress.h"
#include "log.h"

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

void UAnimSequence::ResizeSequence(float NewLength, int32 NewNumFrames, bool bInsert, int32 StartFrame/*inclusive */, int32 EndFrame/*inclusive*/)
{
	int32 OldNumFrames = NumFrames;
	float OldSequenceLength = SequenceLength;

	// verify condition
	NumFrames = NewNumFrames;
	// Update sequence length to match new number of frames.
	SequenceLength = NewLength;
}

void UAnimSequence::PostProcessSequence(bool bForceNewRawDatGuid /*= true*/)
{
	CompressRawAnimData();

	OnRawDataChanged();
}

bool UAnimSequence::CompressRawAnimData()
{
	const float MaxPosDiff = 0.0001f;
	const float MaxAngleDiff = 0.0003f;
	return CompressRawAnimData(MaxPosDiff, MaxAngleDiff);
}

bool UAnimSequence::CompressRawAnimData(float MaxPosDiff, float MaxAngleDiff)
{
	bool bRemovedKeys = false;

	if (AnimationTrackNames.size() > 0 && /*ensureMsgf*/(RawAnimationData.size() > 0/*, TEXT("%s is trying to compress while raw animation is missing"), *GetName()*/))
	{
		for (uint32 TrackIndex = 0; TrackIndex < RawAnimationData.size(); TrackIndex++)
		{
			bRemovedKeys |= CompressRawAnimSequenceTrack(RawAnimationData[TrackIndex], MaxPosDiff, MaxAngleDiff);
		}

		const USkeleton* MySkeleton = GetSkeleton();

		if (MySkeleton)
		{
			bool bCompressScaleKeys = false;
			// go through remove keys if not needed
			for (uint32 TrackIndex = 0; TrackIndex < RawAnimationData.size(); TrackIndex++)
			{
				FRawAnimSequenceTrack const& RawData = RawAnimationData[TrackIndex];
				if (RawData.ScaleKeys.size() > 0)
				{
					// if scale key exists, see if we can just empty it
					if ((RawData.ScaleKeys.size() > 1) || (RawData.ScaleKeys[0].Equals(FVector(1.f)) == false))
					{
						bCompressScaleKeys = true;
						break;
					}
				}
			}

			// if we don't have scale, we should delete all scale keys
			// if you have one track that has scale, we still should support scale, so compress scale
			if (!bCompressScaleKeys)
			{
				// then remove all scale keys
				for (uint32 TrackIndex = 0; TrackIndex < RawAnimationData.size(); TrackIndex++)
				{
					FRawAnimSequenceTrack& RawData = RawAnimationData[TrackIndex];
					RawData.ScaleKeys.clear();
				}
			}
		}

		CompressedTrackOffsets.clear();
		CompressedScaleOffsets.Empty();
	}
	else
	{
		CompressedTrackOffsets.clear();
		CompressedScaleOffsets.Empty();
	}

	return bRemovedKeys;

}

bool UAnimSequence::CompressRawAnimSequenceTrack(FRawAnimSequenceTrack& RawTrack, float MaxPosDiff, float MaxAngleDiff)
{
	bool bRemovedKeys = false;

	// First part is to make sure we have valid input
	bool const bPosTrackIsValid = (RawTrack.PosKeys.size() == 1 || RawTrack.PosKeys.size() == NumFrames);
	if (!bPosTrackIsValid)
	{
		//UE_LOG(LogAnimation, Warning, TEXT("Found non valid position track for %s, %d frames, instead of %d. Chopping!"), *GetName(), RawTrack.PosKeys.Num(), NumFrames);
		bRemovedKeys = true;
		RawTrack.PosKeys.erase(RawTrack.PosKeys.begin() + 1, RawTrack.PosKeys.begin() + 1 + RawTrack.PosKeys.size() - 1);
		//RawTrack.PosKeys.Shrink();
		assert(RawTrack.PosKeys.size() == 1);
	}

	bool const bRotTrackIsValid = (RawTrack.RotKeys.size() == 1 || RawTrack.RotKeys.size() == NumFrames);
	if (!bRotTrackIsValid)
	{
		//UE_LOG(LogAnimation, Warning, TEXT("Found non valid rotation track for %s, %d frames, instead of %d. Chopping!"), *GetName(), RawTrack.RotKeys.Num(), NumFrames);
		bRemovedKeys = true;
		RawTrack.RotKeys.erase(RawTrack.RotKeys.begin() + 1, RawTrack.RotKeys.begin() + 1 + RawTrack.RotKeys.size() - 1);
		//RawTrack.RotKeys.Shrink();
		assert(RawTrack.RotKeys.size() == 1);
	}

	// scale keys can be empty, and that is valid 
	bool const bScaleTrackIsValid = (RawTrack.ScaleKeys.size() == 0 || RawTrack.ScaleKeys.size() == 1 || RawTrack.ScaleKeys.size() == NumFrames);
	if (!bScaleTrackIsValid)
	{
		//UE_LOG(LogAnimation, Warning, TEXT("Found non valid Scaleation track for %s, %d frames, instead of %d. Chopping!"), *GetName(), RawTrack.ScaleKeys.Num(), NumFrames);
		bRemovedKeys = true;
		RawTrack.ScaleKeys.erase(RawTrack.ScaleKeys.begin() + 1, RawTrack.ScaleKeys.begin() + 1 + RawTrack.ScaleKeys.size() - 1);
		//RawTrack.ScaleKeys.Shrink();
		assert(RawTrack.ScaleKeys.size() == 1);
	}

	// Second part is actual compression.

	// Check variation of position keys
	if ((RawTrack.PosKeys.size() > 1) && (MaxPosDiff >= 0.0f))
	{
		FVector FirstPos = RawTrack.PosKeys[0];
		bool bFramesIdentical = true;
		for (uint32 j = 1; j < RawTrack.PosKeys.size() && bFramesIdentical; j++)
		{
			if ((FirstPos - RawTrack.PosKeys[j]).SizeSquared() > FMath::Square(MaxPosDiff))
			{
				bFramesIdentical = false;
			}
		}

		// If all keys are the same, remove all but first frame
		if (bFramesIdentical)
		{
			bRemovedKeys = true;
			RawTrack.PosKeys.erase(RawTrack.PosKeys.begin() + 1, RawTrack.PosKeys.end());
			//RawTrack.PosKeys.Shrink();
			assert(RawTrack.PosKeys.size() == 1);
		}
	}

	// Check variation of rotational keys
	if ((RawTrack.RotKeys.size() > 1) && (MaxAngleDiff >= 0.0f))
	{
		FQuat FirstRot = RawTrack.RotKeys[0];
		bool bFramesIdentical = true;
		for (uint32 j = 1; j < RawTrack.RotKeys.size() && bFramesIdentical; j++)
		{
			if (FQuat::Error(FirstRot, RawTrack.RotKeys[j]) > MaxAngleDiff)
			{
				bFramesIdentical = false;
			}
		}

		// If all keys are the same, remove all but first frame
		if (bFramesIdentical)
		{
			bRemovedKeys = true;
			RawTrack.RotKeys.erase(RawTrack.RotKeys.begin() + 1, RawTrack.RotKeys.end());
			//RawTrack.RotKeys.Shrink();
			assert(RawTrack.RotKeys.size() == 1);
		}
	}

	float MaxScaleDiff = 0.0001f;

	// Check variation of Scaleition keys
	if ((RawTrack.ScaleKeys.size() > 1) && (MaxScaleDiff >= 0.0f))
	{
		FVector FirstScale = RawTrack.ScaleKeys[0];
		bool bFramesIdentical = true;
		for (uint32 j = 1; j < RawTrack.ScaleKeys.size() && bFramesIdentical; j++)
		{
			if ((FirstScale - RawTrack.ScaleKeys[j]).SizeSquared() > FMath::Square(MaxScaleDiff))
			{
				bFramesIdentical = false;
			}
		}

		// If all keys are the same, remove all but first frame
		if (bFramesIdentical)
		{
			bRemovedKeys = true;
			RawTrack.ScaleKeys.erase(RawTrack.ScaleKeys.begin()+1, RawTrack.ScaleKeys.end());
			//RawTrack.ScaleKeys.Shrink();
			assert(RawTrack.ScaleKeys.size() == 1);
		}
	}

	return bRemovedKeys;
}


void UAnimSequence::OnRawDataChanged()
{
	CompressedTrackOffsets.clear();
	CompressedScaleOffsets.Empty();
	CompressedByteStream.clear();
	bUseRawDataOnly = true;

	RequestSyncAnimRecompression(false);
}

void UAnimSequence::RequestAnimCompression(bool bAsyncCompression, bool bAllowAlternateCompressor /*= false*/, bool bOutput /*= false*/)
{
	std::shared_ptr<FAnimCompressContext> CompressContext = std::make_shared<FAnimCompressContext>(bAllowAlternateCompressor, bOutput);
	RequestAnimCompression(bAsyncCompression, CompressContext);
}

void UAnimSequence::RequestAnimCompression(bool bAsyncCompression, std::shared_ptr<FAnimCompressContext> CompressContext)
{
	USkeleton* CurrentSkeleton = GetSkeleton();
	if (CurrentSkeleton == nullptr)
	{
		bUseRawDataOnly = true;
		return;
	}
	const bool bDoCompressionInPlace = false;//FUObjectThreadContext::Get().IsRoutingPostLoad;
	std::vector<uint8> OutData;
	FDerivedDataAnimationCompression* AnimCompressor = new FDerivedDataAnimationCompression(this, CompressContext, bDoCompressionInPlace);

	AnimCompressor->Build(OutData);

	delete AnimCompressor;
	AnimCompressor = nullptr;
}

bool UAnimSequence::IsCompressedDataValid() const
{
	return CompressedByteStream.size() > 0 || RawAnimationData.size() == 0 || (TranslationCompressionFormat == ACF_Identity && RotationCompressionFormat == ACF_Identity && ScaleCompressionFormat == ACF_Identity);
}

int32 UAnimSequence::AddNewRawTrack(std::string TrackName, FRawAnimSequenceTrack* TrackData /*= nullptr*/)
{
	const int32 SkeletonIndex = GetSkeleton() ? GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(TrackName) : INDEX_NONE;

	if (SkeletonIndex != INDEX_NONE)
	{
		int32 TrackIndex = IndexOfByKey(AnimationTrackNames,TrackName);
		if (TrackIndex != INDEX_NONE)
		{
			if (TrackData)
			{
				RawAnimationData[TrackIndex] = *TrackData;
			}
			return TrackIndex;
		}

		assert(AnimationTrackNames.size() == RawAnimationData.size());
		TrackIndex = AnimationTrackNames.size();
		AnimationTrackNames.push_back(TrackName);
		TrackToSkeletonMapTable.push_back(FTrackToSkeletonMap(SkeletonIndex));
		if (TrackData)
		{
			RawAnimationData.push_back(*TrackData);
		}
		else
		{
			RawAnimationData.push_back(FRawAnimSequenceTrack());
		}
		return TrackIndex;
	}
	return INDEX_NONE;
}

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

