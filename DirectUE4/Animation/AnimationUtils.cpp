#include "AnimationUtils.h"
#include "AnimSequence.h"
#include "AnimCompress.h"

void FAnimationUtils::BuildSkeletonMetaData(USkeleton* Skeleton, std::vector<FBoneData>& OutBoneData)
{

}

void FAnimationUtils::ComputeCompressionError(const UAnimSequence* AnimSeq, const std::vector<FBoneData>& BoneData, AnimationErrorStats& ErrorStats)
{

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

