#include "AnimationUtils.h"
#include "AnimSequence.h"
#include "AnimCompress.h"

void FAnimationUtils::BuildSkeletonMetaData(USkeleton* Skeleton, std::vector<FBoneData>& OutBoneData)
{
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	const std::vector<FTransform> & SkeletonRefPose = Skeleton->GetRefLocalPoses();
	const int32 NumBones = RefSkeleton.GetNum();

	OutBoneData.clear();
	OutBoneData.resize(NumBones);

	const std::vector<std::string>& KeyEndEffectorsMatchNameArray = {"IK","eye","weapon","hand","attach","camera" };

	for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
	{
		FBoneData& BoneData = OutBoneData[BoneIndex];

		// Copy over data from the skeleton.
		const FTransform& SrcTransform = SkeletonRefPose[BoneIndex];

		assert(!SrcTransform.ContainsNaN());
		assert(SrcTransform.IsRotationNormalized());

		BoneData.Orientation = SrcTransform.GetRotation();
		BoneData.Position = SrcTransform.GetTranslation();
		BoneData.Name = RefSkeleton.GetBoneName(BoneIndex);

		if (BoneIndex > 0)
		{
			// Compute ancestry.
			int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
			BoneData.BonesToRoot.push_back(ParentIndex);
			while (ParentIndex > 0)
			{
				ParentIndex = RefSkeleton.GetParentIndex(ParentIndex);
				BoneData.BonesToRoot.push_back(ParentIndex);
			}
		}

		// See if a Socket is attached to that bone
		BoneData.bHasSocket = false;
		// @todo anim: socket isn't moved to Skeleton yet, but this code needs better testing
// 		for (int32 SocketIndex = 0; SocketIndex < Skeleton->Sockets.Num(); SocketIndex++)
// 		{
// 			USkeletalMeshSocket* Socket = Skeleton->Sockets[SocketIndex];
// 			if (Socket && Socket->BoneName == RefSkeleton.GetBoneName(BoneIndex))
// 			{
// 				BoneData.bHasSocket = true;
// 				break;
// 			}
// 		}
	}

	// Enumerate children (bones that refer to this bone as parent).
	for (uint32 BoneIndex = 0; BoneIndex < OutBoneData.size(); ++BoneIndex)
	{
		FBoneData& BoneData = OutBoneData[BoneIndex];
		// Exclude the root bone as it is the child of nothing.
		for (uint32 BoneIndex2 = 1; BoneIndex2 < OutBoneData.size(); ++BoneIndex2)
		{
			if (OutBoneData[BoneIndex2].GetParent() == BoneIndex)
			{
				BoneData.Children.push_back(BoneIndex2);
			}
		}
	}

	for (uint32 BoneIndex = 0; BoneIndex < OutBoneData.size(); ++BoneIndex)
	{
		FBoneData& BoneData = OutBoneData[BoneIndex];
		if (BoneData.IsEndEffector())
		{
			// End effectors have themselves as an ancestor.
			BoneData.EndEffectors.push_back(BoneIndex);
			// Add the end effector to the list of end effectors of all ancestors.
			for (uint32 i = 0; i < BoneData.BonesToRoot.size(); ++i)
			{
				const int32 AncestorIndex = BoneData.BonesToRoot[i];
				OutBoneData[AncestorIndex].EndEffectors.push_back(BoneIndex);
			}

			for (uint32 MatchIndex = 0; MatchIndex < KeyEndEffectorsMatchNameArray.size(); MatchIndex++)
			{
				// See if this bone has been defined as a 'key' end effector
				std::string BoneString(BoneData.Name);
				if (BoneString.find(KeyEndEffectorsMatchNameArray[MatchIndex]) != std::string::npos)
				{
					BoneData.bKeyEndEffector = true;
					break;
				}
			}
		}
	}
}

void FAnimationUtils::ComputeCompressionError(const UAnimSequence* AnimSeq, const std::vector<FBoneData>& BoneData, AnimationErrorStats& ErrorStats)
{
	ErrorStats.AverageError = 0.0f;
	ErrorStats.MaxError = 0.0f;
	ErrorStats.MaxErrorBone = 0;
	ErrorStats.MaxErrorTime = 0.0f;
	int32 MaxErrorTrack = -1;
}

void FAnimationUtils::CompressAnimSequence(UAnimSequence* AnimSeq, FAnimCompressContext& CompressContext)
{
	if (!AnimSeq->GetSkeleton())
	{
		return;
	}

	float MasterTolerance = 1.0f;// GetAlternativeCompressionThreshold();

	bool bForceBelowThreshold = false;// AnimSetting->bForceBelowThreshold;
	bool bFirstRecompressUsingCurrentOrDefault = true;// AnimSetting->bFirstRecompressUsingCurrentOrDefault;
	bool bRaiseMaxErrorToExisting = false;// AnimSetting->bRaiseMaxErrorToExisting;
	// If we don't allow alternate compressors, and just want to recompress with default/existing, then make sure we do so.
	if (!CompressContext.bAllowAlternateCompressor)
	{
		bFirstRecompressUsingCurrentOrDefault = true;
	}

	bool bTryFixedBitwiseCompression = true;//AnimSetting->bTryFixedBitwiseCompression;
	bool bTryPerTrackBitwiseCompression = true;//AnimSetting->bTryPerTrackBitwiseCompression;
	bool bTryLinearKeyRemovalCompression = true;//AnimSetting->bTryLinearKeyRemovalCompression;
	bool bTryIntervalKeyRemoval = true;//AnimSetting->bTryIntervalKeyRemoval;

	CompressAnimSequenceExplicit(
		AnimSeq,
		CompressContext,
		CompressContext.bAllowAlternateCompressor ? MasterTolerance : 0.0f,
		bFirstRecompressUsingCurrentOrDefault,
		bForceBelowThreshold,
		bRaiseMaxErrorToExisting,
		bTryFixedBitwiseCompression,
		bTryPerTrackBitwiseCompression,
		bTryLinearKeyRemovalCompression,
		bTryIntervalKeyRemoval);
}

void FAnimationUtils::CompressAnimSequenceExplicit(
	UAnimSequence* AnimSeq, 
	FAnimCompressContext& CompressContext, 
	float MasterTolerance, 
	bool bFirstRecompressUsingCurrentOrDefault, 
	bool bForceBelowThreshold, 
	bool bRaiseMaxErrorToExisting, 
	bool bTryFixedBitwiseCompression, 
	bool bTryPerTrackBitwiseCompression, 
	bool bTryLinearKeyRemovalCompression, 
	bool bTryIntervalKeyRemoval)
{
	assert(AnimSeq != nullptr);
	USkeleton* Skeleton = AnimSeq->GetSkeleton();
	assert(Skeleton);

	const uint32 NumRawDataTracks = AnimSeq->GetRawAnimationData().size();

	if (NumRawDataTracks > 0)
	{

		const bool bTryAlternateCompressor = MasterTolerance > 0.0f;

		AnimSeq->CompressRawAnimData(-1.0f, -1.0f);

		AnimationErrorStats OriginalErrorStats;
		AnimationErrorStats TrueOriginalErrorStats;
		std::vector<FBoneData> BoneData;

		// Build skeleton metadata to use during the key reduction.
		FAnimationUtils::BuildSkeletonMetaData(Skeleton, BoneData);
		FAnimationUtils::ComputeCompressionError(AnimSeq, BoneData, TrueOriginalErrorStats);

		if ((bFirstRecompressUsingCurrentOrDefault && !bTryAlternateCompressor) || (AnimSeq->CompressedByteStream.size() == 0))
		{
			UAnimCompress* OriginalCompressionAlgorithm = FAnimationUtils::GetDefaultAnimationCompressionAlgorithm();

			//AnimSeq->CompressionScheme = OriginalCompressionAlgorithm;
			OriginalCompressionAlgorithm->Reduce(AnimSeq, CompressContext);
			AnimSeq->SetUseRawDataOnly(false);
			//AfterOriginalRecompression = AnimSeq->GetApproxCompressedSize();

			// figure out our current compression error
			FAnimationUtils::ComputeCompressionError(AnimSeq, BoneData, OriginalErrorStats);
		}
	}
}
static inline UAnimCompress* ConstructDefaultCompressionAlgorithm()
{
	UAnimCompress* NewAlgorithm = new UAnimCompress_BitwiseCompressOnly();
	NewAlgorithm->RotationCompressionFormat = ACF_Float96NoW;// RotationCompressionFormat;
	NewAlgorithm->TranslationCompressionFormat = ACF_None;// TranslationCompressionFormat;
	return NewAlgorithm;
}

UAnimCompress* FAnimationUtils::GetDefaultAnimationCompressionAlgorithm()
{
	static UAnimCompress* SAlgorithm = ConstructDefaultCompressionAlgorithm();
	return SAlgorithm;
}

