#include "AnimCompress.h"
#include "AnimationCompression.h"
#include "AnimEncoding.h"

// Writes the specified data to Seq->CompresedByteStream with four-byte alignment.
#define AC_UnalignedWriteToStream( Src, Len )										\
	{																				\
		const uint32 Ofs = Seq->CompressedByteStream.size();						\
		Seq->CompressedByteStream.insert(Seq->CompressedByteStream.end(),Len,0);	\
		memcpy( Seq->CompressedByteStream.data()+Ofs, (Src), (Len) );				\
	}

static const uint8 AnimationPadSentinel = 85; //(1<<1)+(1<<3)+(1<<5)+(1<<7)


static void PackVectorToStream(
	UAnimSequence* Seq,
	AnimationCompressionFormat TargetTranslationFormat,
	const FVector& Vec,
	const float* Mins,
	const float* Ranges)
{
	if (TargetTranslationFormat == ACF_None)
	{
		AC_UnalignedWriteToStream(&Vec, sizeof(Vec));
	}
	else if (TargetTranslationFormat == ACF_Float96NoW)
	{
		AC_UnalignedWriteToStream(&Vec, sizeof(Vec));
	}
	else if (TargetTranslationFormat == ACF_IntervalFixed32NoW)
	{
		const FVectorIntervalFixed32NoW CompressedVec(Vec, Mins, Ranges);

		AC_UnalignedWriteToStream(&CompressedVec, sizeof(CompressedVec));
	}
}

static void PackQuaternionToStream(
	UAnimSequence* Seq,
	AnimationCompressionFormat TargetRotationFormat,
	const FQuat& Quat,
	const float* Mins,
	const float* Ranges)
{
	if (TargetRotationFormat == ACF_None)
	{
		AC_UnalignedWriteToStream(&Quat, sizeof(FQuat));
	}
	else if (TargetRotationFormat == ACF_Float96NoW)
	{
		const FQuatFloat96NoW QuatFloat96NoW(Quat);
		AC_UnalignedWriteToStream(&QuatFloat96NoW, sizeof(FQuatFloat96NoW));
	}
	else if (TargetRotationFormat == ACF_Fixed32NoW)
	{
		const FQuatFixed32NoW QuatFixed32NoW(Quat);
		AC_UnalignedWriteToStream(&QuatFixed32NoW, sizeof(FQuatFixed32NoW));
	}
	else if (TargetRotationFormat == ACF_Fixed48NoW)
	{
		const FQuatFixed48NoW QuatFixed48NoW(Quat);
		AC_UnalignedWriteToStream(&QuatFixed48NoW, sizeof(FQuatFixed48NoW));
	}
	else if (TargetRotationFormat == ACF_IntervalFixed32NoW)
	{
		const FQuatIntervalFixed32NoW QuatIntervalFixed32NoW(Quat, Mins, Ranges);
		AC_UnalignedWriteToStream(&QuatIntervalFixed32NoW, sizeof(FQuatIntervalFixed32NoW));
	}
	else if (TargetRotationFormat == ACF_Float32NoW)
	{
		const FQuatFloat32NoW QuatFloat32NoW(Quat);
		AC_UnalignedWriteToStream(&QuatFloat32NoW, sizeof(FQuatFloat32NoW));
	}
}

uint8 MakeBitForFlag(uint32 Item, uint32 Position)
{
	assert(Item < 2);
	return Item << Position;
}

FCompressionMemorySummary::FCompressionMemorySummary(bool bInEnabled)
	: bEnabled(bInEnabled)
	, bUsed(false)
	, TotalRaw(0)
	, TotalBeforeCompressed(0)
	, TotalAfterCompressed(0)
	, ErrorTotal(0)
	, ErrorCount(0)
	, AverageError(0)
	, MaxError(0)
	, MaxErrorTime(0)
	, MaxErrorBone(0)
{

}

void FCompressionMemorySummary::GatherPreCompressionStats(UAnimSequence* Seq, int32 ProgressNumerator, int32 ProgressDenominator)
{

}

void FCompressionMemorySummary::GatherPostCompressionStats(UAnimSequence* Seq, std::vector<FBoneData>& BoneData)
{

}

FCompressionMemorySummary::~FCompressionMemorySummary()
{

}

void FAnimCompressContext::GatherPreCompressionStats(UAnimSequence* Seq)
{

}

void FAnimCompressContext::GatherPostCompressionStats(UAnimSequence* Seq, std::vector<FBoneData>& BoneData)
{

}

bool UAnimCompress::Reduce(class UAnimSequence* AnimSeq, bool bOutput)
{
	bool bResult = false;
	USkeleton* AnimSkeleton = AnimSeq->GetSkeleton();
	const bool bSkeletonExistsIfNeeded = (AnimSkeleton || !bNeedsSkeleton);
	if (bSkeletonExistsIfNeeded)
	{
		FAnimCompressContext CompressContext(false, bOutput);
		Reduce(AnimSeq, CompressContext);

		bResult = true;
	}
	return bResult;
}

bool UAnimCompress::Reduce(class UAnimSequence* AnimSeq, FAnimCompressContext& Context)
{
	bool bResult = false;

	std::vector<FBoneData> BoneData;
	FAnimationUtils::BuildSkeletonMetaData(AnimSeq->GetSkeleton(), BoneData);
	Context.GatherPreCompressionStats(AnimSeq);

	// General key reduction.
	DoReduction(AnimSeq, BoneData);

	//AnimSeq->bWasCompressedWithoutTranslations = false; // @fixmelh : bAnimRotationOnly

	//AnimSeq->EncodingPkgVersion = CURRENT_ANIMATION_ENCODING_PACKAGE_VERSION;
	//AnimSeq->MarkPackageDirty();

	// determine the error added by the compression
	Context.GatherPostCompressionStats(AnimSeq, BoneData);
	bResult = true;

	return bResult;
}

void UAnimCompress::FilterTrivialPositionKeys(std::vector<struct FTranslationTrack>& Track, float MaxPosDelta)
{

}

void UAnimCompress::FilterTrivialPositionKeys(struct FTranslationTrack& Track, float MaxPosDelta)
{

}

void UAnimCompress::FilterTrivialRotationKeys(std::vector<struct FRotationTrack>& InputTracks, float MaxRotDelta)
{

}

void UAnimCompress::FilterTrivialRotationKeys(struct FRotationTrack& Track, float MaxRotDelta)
{

}

void UAnimCompress::FilterTrivialScaleKeys(std::vector<struct FScaleTrack>& Track, float MaxScaleDelta)
{

}

void UAnimCompress::FilterTrivialScaleKeys(struct FScaleTrack& Track, float MaxScaleDelta)
{

}

void UAnimCompress::SeparateRawDataIntoTracks(
	const std::vector<struct FRawAnimSequenceTrack>& RawAnimData, 
	float SequenceLength, 
	std::vector<struct FTranslationTrack>& OutTranslationData, 
	std::vector<struct FRotationTrack>& OutRotationData, 
	std::vector<struct FScaleTrack>& OutScaleData)
{

}

void UAnimCompress::FilterAnimRotationOnlyKeys(std::vector<FTranslationTrack>& PositionTracks, UAnimSequence* AnimSeq)
{

}

void UAnimCompress::FilterTrivialKeys(std::vector<struct FTranslationTrack>& PositionTracks, std::vector<struct FRotationTrack>& RotationTracks, std::vector<struct FScaleTrack>& ScaleTracks, float MaxPosDelta, float MaxRotDelta, float MaxScaleDelta)
{
	FilterTrivialRotationKeys(RotationTracks, MaxRotDelta);
	FilterTrivialPositionKeys(PositionTracks, MaxPosDelta);
	FilterTrivialScaleKeys(ScaleTracks, MaxScaleDelta);
}

void PadByteStream(std::vector<uint8>& CompressedByteStream, const int32 Alignment, uint8 sentinel)
{
	int32 Pad = Align(CompressedByteStream.size(), 4) - CompressedByteStream.size();
	for (int32 i = 0; i < Pad; ++i)
	{
		CompressedByteStream.push_back(sentinel);
	}
}

void UAnimCompress::BitwiseCompressAnimationTracks(
	class UAnimSequence* Seq, 
	AnimationCompressionFormat TargetTranslationFormat, 
	AnimationCompressionFormat TargetRotationFormat, 
	AnimationCompressionFormat TargetScaleFormat, 
	const std::vector<FTranslationTrack>& TranslationData, 
	const std::vector<FRotationTrack>& RotationData, 
	const std::vector<FScaleTrack>& ScaleData, 
	bool IncludeKeyTable /*= false*/)
{
	bool bInvalidCompressionFormat = false;
	if (!(TargetTranslationFormat == ACF_None) && !(TargetTranslationFormat == ACF_IntervalFixed32NoW) && !(TargetTranslationFormat == ACF_Float96NoW))
	{
		assert(false);//FMessageDialog::Open(EAppMsgType::Ok, FText::Format(NSLOCTEXT("Engine", "UnknownTranslationCompressionFormat", "Unknown or unsupported translation compression format ({0})"), FText::AsNumber((int32)TargetTranslationFormat)));
		bInvalidCompressionFormat = true;
	}
	if (!(TargetRotationFormat >= ACF_None && TargetRotationFormat < ACF_MAX))
	{
		assert(false);//FMessageDialog::Open(EAppMsgType::Ok, FText::Format(NSLOCTEXT("Engine", "UnknownRotationCompressionFormat", "Unknown or unsupported rotation compression format ({0})"), FText::AsNumber((int32)TargetRotationFormat)));
		bInvalidCompressionFormat = true;
	}
	if (!(TargetScaleFormat == ACF_None) && !(TargetScaleFormat == ACF_IntervalFixed32NoW) && !(TargetScaleFormat == ACF_Float96NoW))
	{
		assert(false);//FMessageDialog::Open(EAppMsgType::Ok, FText::Format(NSLOCTEXT("Engine", "UnknownScaleCompressionFormat", "Unknown or unsupported Scale compression format ({0})"), FText::AsNumber((int32)TargetScaleFormat)));
		bInvalidCompressionFormat = true;
	}

	if (bInvalidCompressionFormat)
	{
		Seq->TranslationCompressionFormat = ACF_None;
		Seq->RotationCompressionFormat = ACF_None;
		Seq->ScaleCompressionFormat = ACF_None;
		Seq->CompressedTrackOffsets.clear();
		Seq->CompressedScaleOffsets.Empty();
		Seq->CompressedByteStream.clear();
	}
	else
	{
		Seq->RotationCompressionFormat = TargetRotationFormat;
		Seq->TranslationCompressionFormat = TargetTranslationFormat;
		Seq->ScaleCompressionFormat = TargetScaleFormat;

		assert(TranslationData.size() == RotationData.size());
		const uint32 NumTracks = RotationData.size();
		const bool bHasScale = ScaleData.size() > 0;

		Seq->CompressedTrackOffsets.clear();
		Seq->CompressedTrackOffsets.resize(NumTracks * 4);

		// just empty it since there is chance this can be 0
		Seq->CompressedScaleOffsets.Empty();
		// only do this if Scale exists;
		if (bHasScale)
		{
			Seq->CompressedScaleOffsets.SetStripSize(2);
			Seq->CompressedScaleOffsets.AddUninitialized(NumTracks);
		}

		Seq->CompressedByteStream.clear();

		for (uint32 TrackIndex = 0; TrackIndex < NumTracks; ++TrackIndex)
		{
			const FTranslationTrack& SrcTrans = TranslationData[TrackIndex];

			const uint32 OffsetTrans = Seq->CompressedByteStream.size();
			const uint32 NumKeysTrans = SrcTrans.PosKeys.size();

			assert((OffsetTrans % 4) == 0);// , TEXT("CompressedByteStream not aligned to four bytes"));

			Seq->CompressedTrackOffsets[TrackIndex * 4] = OffsetTrans;
			Seq->CompressedTrackOffsets[TrackIndex * 4 + 1] = NumKeysTrans;

			FBox PositionBounds(SrcTrans.PosKeys);

			float TransMins[3] = { PositionBounds.Min.X, PositionBounds.Min.Y, PositionBounds.Min.Z };
			float TransRanges[3] = { PositionBounds.Max.X - PositionBounds.Min.X, PositionBounds.Max.Y - PositionBounds.Min.Y, PositionBounds.Max.Z - PositionBounds.Min.Z };
			if (TransRanges[0] == 0.f) { TransRanges[0] = 1.f; }
			if (TransRanges[1] == 0.f) { TransRanges[1] = 1.f; }
			if (TransRanges[2] == 0.f) { TransRanges[2] = 1.f; }

			if (NumKeysTrans > 1)
			{
				// Write the mins and ranges if they'll be used on the other side
				if (TargetTranslationFormat == ACF_IntervalFixed32NoW)
				{
					AC_UnalignedWriteToStream(TransMins, sizeof(float) * 3);
					AC_UnalignedWriteToStream(TransRanges, sizeof(float) * 3);
				}

				// Pack the positions into the stream
				for (uint32 KeyIndex = 0; KeyIndex < NumKeysTrans; ++KeyIndex)
				{
					const FVector& Vec = SrcTrans.PosKeys[KeyIndex];
					PackVectorToStream(Seq, TargetTranslationFormat, Vec, TransMins, TransRanges);
				}

				if (IncludeKeyTable)
				{
					// Align to four bytes.
					PadByteStream(Seq->CompressedByteStream, 4, AnimationPadSentinel);

					// write the key table
					const int32 NumFrames = Seq->NumFrames;
					const int32 LastFrame = Seq->NumFrames - 1;
					const size_t FrameSize = Seq->NumFrames > 0xff ? sizeof(uint16) : sizeof(uint8);
					const float FrameRate = NumFrames / Seq->SequenceLength;

					const int32 TableSize = NumKeysTrans * FrameSize;
					const int32 TableDwords = (TableSize + 3) >> 2;
					const uint32 StartingOffset = Seq->CompressedByteStream.size();

					for (uint32 KeyIndex = 0; KeyIndex < NumKeysTrans; ++KeyIndex)
					{
						// write the frame values for each key
						float KeyTime = SrcTrans.Times[KeyIndex];
						float FrameTime = KeyTime * FrameRate;
						int32 FrameIndex = FMath::Clamp(FMath::TruncToInt(FrameTime), 0, LastFrame);
						AC_UnalignedWriteToStream(&FrameIndex, FrameSize);
					}

					// Align to four bytes. Padding with 0's to round out the key table
					PadByteStream(Seq->CompressedByteStream, 4, 0);

					const int32 EndingOffset = Seq->CompressedByteStream.size();
					assert((EndingOffset - StartingOffset) == (TableDwords * 4));
				}
			}
			else if (NumKeysTrans == 1)
			{
				// A single translation key gets written out a single uncompressed float[3].
				AC_UnalignedWriteToStream(&(SrcTrans.PosKeys[0]), sizeof(FVector));
			}
			else
			{
				//UE_LOG(LogAnimationCompression, Warning, TEXT("When compressing %s track %i: no translation keys"), *Seq->GetName(), TrackIndex);
			}

			// Align to four bytes.
			PadByteStream(Seq->CompressedByteStream, 4, AnimationPadSentinel);

			// Compress rotation data.
			const FRotationTrack& SrcRot = RotationData[TrackIndex];
			const uint32 OffsetRot = Seq->CompressedByteStream.size();
			const uint32 NumKeysRot = SrcRot.RotKeys.size();

			assert((OffsetRot % 4) == 0);//, TEXT("CompressedByteStream not aligned to four bytes"));
			Seq->CompressedTrackOffsets[TrackIndex * 4 + 2] = OffsetRot;
			Seq->CompressedTrackOffsets[TrackIndex * 4 + 3] = NumKeysRot;

			if (NumKeysRot > 1)
			{
				// Calculate the min/max of the XYZ components of the quaternion
				float MinX = 1.f;
				float MinY = 1.f;
				float MinZ = 1.f;
				float MaxX = -1.f;
				float MaxY = -1.f;
				float MaxZ = -1.f;
				for (uint32 KeyIndex = 0; KeyIndex < SrcRot.RotKeys.size(); ++KeyIndex)
				{
					FQuat Quat(SrcRot.RotKeys[KeyIndex]);
					if (Quat.W < 0.f)
					{
						Quat.X = -Quat.X;
						Quat.Y = -Quat.Y;
						Quat.Z = -Quat.Z;
						Quat.W = -Quat.W;
					}
					Quat.Normalize();

					MinX = FMath::Min(MinX, Quat.X);
					MaxX = FMath::Max(MaxX, Quat.X);
					MinY = FMath::Min(MinY, Quat.Y);
					MaxY = FMath::Max(MaxY, Quat.Y);
					MinZ = FMath::Min(MinZ, Quat.Z);
					MaxZ = FMath::Max(MaxZ, Quat.Z);
				}
				const float Mins[3] = { MinX,		MinY,		MinZ };
				float Ranges[3] = { MaxX - MinX,	MaxY - MinY,	MaxZ - MinZ };
				if (Ranges[0] == 0.f) { Ranges[0] = 1.f; }
				if (Ranges[1] == 0.f) { Ranges[1] = 1.f; }
				if (Ranges[2] == 0.f) { Ranges[2] = 1.f; }

				// Write the mins and ranges if they'll be used on the other side
				if (TargetRotationFormat == ACF_IntervalFixed32NoW)
				{
					AC_UnalignedWriteToStream(Mins, sizeof(float) * 3);
					AC_UnalignedWriteToStream(Ranges, sizeof(float) * 3);
				}

				// n elements of the compressed type.
				for (uint32 KeyIndex = 0; KeyIndex < SrcRot.RotKeys.size(); ++KeyIndex)
				{
					const FQuat& Quat = SrcRot.RotKeys[KeyIndex];
					PackQuaternionToStream(Seq, TargetRotationFormat, Quat, Mins, Ranges);
				}

				// n elements of frame indices
				if (IncludeKeyTable)
				{
					// Align to four bytes.
					PadByteStream(Seq->CompressedByteStream, 4, AnimationPadSentinel);

					// write the key table
					const int32 NumFrames = Seq->NumFrames;
					const int32 LastFrame = Seq->NumFrames - 1;
					const size_t FrameSize = Seq->NumFrames > 0xff ? sizeof(uint16) : sizeof(uint8);
					const float FrameRate = NumFrames / Seq->SequenceLength;

					const int32 TableSize = NumKeysRot * FrameSize;
					const int32 TableDwords = (TableSize + 3) >> 2;
					const uint32 StartingOffset = Seq->CompressedByteStream.size();

					for (uint32 KeyIndex = 0; KeyIndex < NumKeysRot; ++KeyIndex)
					{
						// write the frame values for each key
						float KeyTime = SrcRot.Times[KeyIndex];
						float FrameTime = KeyTime * FrameRate;
						int32 FrameIndex = FMath::Clamp(FMath::TruncToInt(FrameTime), 0, LastFrame);
						AC_UnalignedWriteToStream(&FrameIndex, FrameSize);
					}

					// Align to four bytes. Padding with 0's to round out the key table
					PadByteStream(Seq->CompressedByteStream, 4, 0);

					const uint32 EndingOffset = Seq->CompressedByteStream.size();
					assert((EndingOffset - StartingOffset) == (TableDwords * 4));

				}
			}
			else if (NumKeysRot == 1)
			{
				// For a rotation track of n=1 keys, the single key is packed as an FQuatFloat96NoW.
				const FQuat& Quat = SrcRot.RotKeys[0];
				const FQuatFloat96NoW QuatFloat96NoW(Quat);
				AC_UnalignedWriteToStream(&QuatFloat96NoW, sizeof(FQuatFloat96NoW));
			}
			else
			{
				//UE_LOG(LogAnimationCompression, Warning, TEXT("When compressing %s track %i: no rotation keys"), *Seq->GetName(), TrackIndex);
			}


			// Align to four bytes.
			PadByteStream(Seq->CompressedByteStream, 4, AnimationPadSentinel);

			// we also should do this only when scale exists. 
			if (bHasScale)
			{
				const FScaleTrack& SrcScale = ScaleData[TrackIndex];

				const uint32 OffsetScale = Seq->CompressedByteStream.size();
				const uint32 NumKeysScale = SrcScale.ScaleKeys.size();

				assert((OffsetScale % 4) == 0);// , TEXT("CompressedByteStream not aligned to four bytes"));
				Seq->CompressedScaleOffsets.SetOffsetData(TrackIndex, 0, OffsetScale);
				Seq->CompressedScaleOffsets.SetOffsetData(TrackIndex, 1, NumKeysScale);

				// Calculate the bounding box of the Scalelation keys
				FBox ScaleBoundsBounds(SrcScale.ScaleKeys);

				float ScaleMins[3] = { ScaleBoundsBounds.Min.X, ScaleBoundsBounds.Min.Y, ScaleBoundsBounds.Min.Z };
				float ScaleRanges[3] = { ScaleBoundsBounds.Max.X - ScaleBoundsBounds.Min.X, ScaleBoundsBounds.Max.Y - ScaleBoundsBounds.Min.Y, ScaleBoundsBounds.Max.Z - ScaleBoundsBounds.Min.Z };
				// @todo - this isn't good for scale 
				// 			if ( ScaleRanges[0] == 0.f ) { ScaleRanges[0] = 1.f; }
				// 			if ( ScaleRanges[1] == 0.f ) { ScaleRanges[1] = 1.f; }
				// 			if ( ScaleRanges[2] == 0.f ) { ScaleRanges[2] = 1.f; }

				if (NumKeysScale > 1)
				{
					// Write the mins and ranges if they'll be used on the other side
					if (TargetScaleFormat == ACF_IntervalFixed32NoW)
					{
						AC_UnalignedWriteToStream(ScaleMins, sizeof(float) * 3);
						AC_UnalignedWriteToStream(ScaleRanges, sizeof(float) * 3);
					}

					// Pack the positions into the stream
					for (uint32 KeyIndex = 0; KeyIndex < NumKeysScale; ++KeyIndex)
					{
						const FVector& Vec = SrcScale.ScaleKeys[KeyIndex];
						PackVectorToStream(Seq, TargetScaleFormat, Vec, ScaleMins, ScaleRanges);
					}

					if (IncludeKeyTable)
					{
						// Align to four bytes.
						PadByteStream(Seq->CompressedByteStream, 4, AnimationPadSentinel);

						// write the key table
						const int32 NumFrames = Seq->NumFrames;
						const int32 LastFrame = Seq->NumFrames - 1;
						const size_t FrameSize = Seq->NumFrames > 0xff ? sizeof(uint16) : sizeof(uint8);
						const float FrameRate = NumFrames / Seq->SequenceLength;

						const int32 TableSize = NumKeysScale * FrameSize;
						const int32 TableDwords = (TableSize + 3) >> 2;
						const uint32 StartingOffset = Seq->CompressedByteStream.size();

						for (uint32 KeyIndex = 0; KeyIndex < NumKeysScale; ++KeyIndex)
						{
							// write the frame values for each key
							float KeyTime = SrcScale.Times[KeyIndex];
							float FrameTime = KeyTime * FrameRate;
							int32 FrameIndex = FMath::Clamp(FMath::TruncToInt(FrameTime), 0, LastFrame);
							AC_UnalignedWriteToStream(&FrameIndex, FrameSize);
						}

						// Align to four bytes. Padding with 0's to round out the key table
						PadByteStream(Seq->CompressedByteStream, 4, 0);

						const uint32 EndingOffset = Seq->CompressedByteStream.size();
						assert((EndingOffset - StartingOffset) == (TableDwords * 4));
					}
				}
				else if (NumKeysScale == 1)
				{
					// A single Scalelation key gets written out a single uncompressed float[3].
					AC_UnalignedWriteToStream(&(SrcScale.ScaleKeys[0]), sizeof(FVector));
				}
				else
				{
					//UE_LOG(LogAnimationCompression, Warning, TEXT("When compressing %s track %i: no Scale keys"), *Seq->GetName(), TrackIndex);
				}

				// Align to four bytes.
				PadByteStream(Seq->CompressedByteStream, 4, AnimationPadSentinel);
			}
		}
	}
}

void UAnimCompress_BitwiseCompressOnly::DoReduction(class UAnimSequence* AnimSeq, const std::vector<class FBoneData>& BoneData)
{
	std::vector<FTranslationTrack> TranslationData;
	std::vector<FRotationTrack> RotationData;
	std::vector<FScaleTrack> ScaleData;
	SeparateRawDataIntoTracks(AnimSeq->GetRawAnimationData(), AnimSeq->SequenceLength, TranslationData, RotationData, ScaleData);

	// Remove Translation Keys from tracks marked bAnimRotationOnly
	FilterAnimRotationOnlyKeys(TranslationData, AnimSeq);

	// remove obviously redundant keys from the source data
	FilterTrivialKeys(TranslationData, RotationData, ScaleData, TRANSLATION_ZEROING_THRESHOLD, QUATERNION_ZEROING_THRESHOLD, SCALE_ZEROING_THRESHOLD);

	// bitwise compress the tracks into the anim sequence buffers
	BitwiseCompressAnimationTracks(
		AnimSeq,
		static_cast<AnimationCompressionFormat>(TranslationCompressionFormat),
		static_cast<AnimationCompressionFormat>(RotationCompressionFormat),
		static_cast<AnimationCompressionFormat>(ScaleCompressionFormat),
		TranslationData,
		RotationData,
		ScaleData);

	// record the proper runtime decompressor to use
	AnimSeq->KeyEncodingFormat = AKF_ConstantKeyLerp;
	AnimationFormat_SetInterfaceLinks(*AnimSeq);
	//AnimSeq->CompressionScheme = static_cast<UAnimCompress*>(StaticDuplicateObject(this, AnimSeq));
}
