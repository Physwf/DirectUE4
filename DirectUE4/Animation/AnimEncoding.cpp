#include "AnimEncoding.h"
#include "AnimEncoding_ConstantKeyLerp.h"
#include "AnimSequence.h"
#include "AnimationCompression.h"

const int32 CompressedTranslationStrides[ACF_MAX] =
{
	sizeof(float),						// ACF_None					(float X, float Y, float Z)
	sizeof(float),						// ACF_Float96NoW			(float X, float Y, float Z)
	sizeof(float),						// ACF_Fixed48NoW			(Illegal value for translation)
	sizeof(FVectorIntervalFixed32NoW),	// ACF_IntervalFixed32NoW	(compressed to 11-11-10 per-component interval fixed point)
	sizeof(float),						// ACF_Fixed32NoW			(Illegal value for translation)
	sizeof(float),						// ACF_Float32NoW			(Illegal value for translation)
	0									// ACF_Identity
};

const int32 CompressedTranslationNum[ACF_MAX] =
{
	3,	// ACF_None					(float X, float Y, float Z)
	3,	// ACF_Float96NoW			(float X, float Y, float Z)
	3,	// ACF_Fixed48NoW			(Illegal value for translation)
	1,	// ACF_IntervalFixed32NoW	(compressed to 11-11-10 per-component interval fixed point)
	3,	// ACF_Fixed32NoW			(Illegal value for translation)
	3,	// ACF_Float32NoW			(Illegal value for translation)
	0	// ACF_Identity
};

const int32 CompressedRotationStrides[ACF_MAX] =
{
	sizeof(float),						// ACF_None					(FQuats are serialized per element hence sizeof(float) rather than sizeof(FQuat).
	sizeof(float),						// ACF_Float96NoW			(FQuats with one component dropped and the remaining three uncompressed at 32bit floating point each
	sizeof(uint16),						// ACF_Fixed48NoW			(FQuats with one component dropped and the remaining three compressed to 16-16-16 fixed point.
	sizeof(FQuatIntervalFixed32NoW),	// ACF_IntervalFixed32NoW	(FQuats with one component dropped and the remaining three compressed to 11-11-10 per-component interval fixed point.
	sizeof(FQuatFixed32NoW),			// ACF_Fixed32NoW			(FQuats with one component dropped and the remaining three compressed to 11-11-10 fixed point.
	sizeof(FQuatFloat32NoW),			// ACF_Float32NoW			(FQuats with one component dropped and the remaining three compressed to 11-11-10 floating point.
	0	// ACF_Identity
};

const int32 CompressedRotationNum[ACF_MAX] =
{
	4,	// ACF_None					(FQuats are serialized per element hence sizeof(float) rather than sizeof(FQuat).
	3,	// ACF_Float96NoW			(FQuats with one component dropped and the remaining three uncompressed at 32bit floating point each
	3,	// ACF_Fixed48NoW			(FQuats with one component dropped and the remaining three compressed to 16-16-16 fixed point.
	1,	// ACF_IntervalFixed32NoW	(FQuats with one component dropped and the remaining three compressed to 11-11-10 per-component interval fixed point.
	1,	// ACF_Fixed32NoW			(FQuats with one component dropped and the remaining three compressed to 11-11-10 fixed point.
	1,  // ACF_Float32NoW			(FQuats with one component dropped and the remaining three compressed to 11-11-10 floating point.
	0	// ACF_Identity
};

const uint8 PerTrackNumComponentTable[ACF_MAX * 8] =
{
	4,4,4,4,4,4,4,4,	// ACF_None
	3,1,1,2,1,2,2,3,	// ACF_Float96NoW (0 is special, as uncompressed rotation gets 'mis'encoded with 0 instead of 7, so it's treated as a 3; a genuine 0 would use ACF_Identity)
	3,1,1,2,1,2,2,3,	// ACF_Fixed48NoW (ditto)
	6,2,2,4,2,4,4,6,	// ACF_IntervalFixed32NoW (special, indicates number of interval pairs stored in the fixed track)
	1,1,1,1,1,1,1,1,	// ACF_Fixed32NoW
	1,1,1,1,1,1,1,1,	// ACF_Float32NoW
	0,0,0,0,0,0,0,0		// ACF_Identity
};

const int32 CompressedScaleStrides[ACF_MAX] =
{
	sizeof(float),						// ACF_None					(float X, float Y, float Z)
	sizeof(float),						// ACF_Float96NoW			(float X, float Y, float Z)
	sizeof(float),						// ACF_Fixed48NoW			(Illegal value for Scale)
	sizeof(FVectorIntervalFixed32NoW),	// ACF_IntervalFixed32NoW	(compressed to 11-11-10 per-component interval fixed point)
	sizeof(float),						// ACF_Fixed32NoW			(Illegal value for Scale)
	sizeof(float),						// ACF_Float32NoW			(Illegal value for Scale)
	0									// ACF_Identity
};

const int32 CompressedScaleNum[ACF_MAX] =
{
	3,	// ACF_None					(float X, float Y, float Z)
	3,	// ACF_Float96NoW			(float X, float Y, float Z)
	3,	// ACF_Fixed48NoW			(Illegal value for Scale)
	1,	// ACF_IntervalFixed32NoW	(compressed to 11-11-10 per-component interval fixed point)
	3,	// ACF_Fixed32NoW			(Illegal value for Scale)
	3,	// ACF_Float32NoW			(Illegal value for Scale)
	0	// ACF_Identity
};

void AnimationFormat_GetBoneAtom(FTransform& OutAtom, const UAnimSequence& Seq, int32 TrackIndex, float Time)
{
	assert(Seq.RotationCodec != NULL);
	((AnimEncoding*)Seq.RotationCodec)->GetBoneAtom(OutAtom, Seq, TrackIndex, Time);
}

void AnimationFormat_GetAnimationPose(
	FTransformArray& Atoms, 
	const BoneTrackArray& RotationPairs,
	const BoneTrackArray& TranslationPairs,
	const BoneTrackArray& ScalePairs,
	const UAnimSequence& Seq, 
	float Time)
{
	// decompress the translation component using the proper method
	assert(Seq.TranslationCodec != NULL);
	if (TranslationPairs.size() > 0)
	{
		((AnimEncoding*)Seq.TranslationCodec)->GetPoseTranslations(Atoms, TranslationPairs, Seq, Time);
	}

	// decompress the rotation component using the proper method
	assert(Seq.RotationCodec != NULL);
	((AnimEncoding*)Seq.RotationCodec)->GetPoseRotations(Atoms, RotationPairs, Seq, Time);

	assert(Seq.ScaleCodec != NULL);
	// we allow scale key to be empty
	if (Seq.CompressedScaleOffsets.IsValid())
	{
		((AnimEncoding*)Seq.ScaleCodec)->GetPoseScales(Atoms, ScalePairs, Seq, Time);
	}
}

void AnimationFormat_SetInterfaceLinks(UAnimSequence& Seq)
{
	Seq.TranslationCodec = NULL;
	Seq.RotationCodec = NULL;
	Seq.ScaleCodec = NULL;

	if (Seq.KeyEncodingFormat == AKF_ConstantKeyLerp)
	{
		static AEFConstantKeyLerp<ACF_None>					AEFConstantKeyLerp_None;
		static AEFConstantKeyLerp<ACF_Float96NoW>			AEFConstantKeyLerp_Float96NoW;
		static AEFConstantKeyLerp<ACF_Fixed48NoW>			AEFConstantKeyLerp_Fixed48NoW;
		static AEFConstantKeyLerp<ACF_IntervalFixed32NoW>	AEFConstantKeyLerp_IntervalFixed32NoW;
		static AEFConstantKeyLerp<ACF_Fixed32NoW>			AEFConstantKeyLerp_Fixed32NoW;
		static AEFConstantKeyLerp<ACF_Float32NoW>			AEFConstantKeyLerp_Float32NoW;
		static AEFConstantKeyLerp<ACF_Identity>				AEFConstantKeyLerp_Identity;

		// setup translation codec
		switch (Seq.TranslationCompressionFormat)
		{
		case ACF_None:
			Seq.TranslationCodec = &AEFConstantKeyLerp_None;
			break;
		case ACF_Float96NoW:
			Seq.TranslationCodec = &AEFConstantKeyLerp_Float96NoW;
			break;
		case ACF_IntervalFixed32NoW:
			Seq.TranslationCodec = &AEFConstantKeyLerp_IntervalFixed32NoW;
			break;
		case ACF_Identity:
			Seq.TranslationCodec = &AEFConstantKeyLerp_Identity;
			break;

		default:
			assert(false);// UE_LOG(LogAnimationCompression, Fatal, TEXT("%i: unknown or unsupported translation compression"), (int32)Seq.TranslationCompressionFormat);
		};

		// setup rotation codec
		switch (Seq.RotationCompressionFormat)
		{
		case ACF_None:
			Seq.RotationCodec = &AEFConstantKeyLerp_None;
			break;
		case ACF_Float96NoW:
			Seq.RotationCodec = &AEFConstantKeyLerp_Float96NoW;
			break;
		case ACF_Fixed48NoW:
			Seq.RotationCodec = &AEFConstantKeyLerp_Fixed48NoW;
			break;
		case ACF_IntervalFixed32NoW:
			Seq.RotationCodec = &AEFConstantKeyLerp_IntervalFixed32NoW;
			break;
		case ACF_Fixed32NoW:
			Seq.RotationCodec = &AEFConstantKeyLerp_Fixed32NoW;
			break;
		case ACF_Float32NoW:
			Seq.RotationCodec = &AEFConstantKeyLerp_Float32NoW;
			break;
		case ACF_Identity:
			Seq.RotationCodec = &AEFConstantKeyLerp_Identity;
			break;

		default:
			assert(false);// UE_LOG(LogAnimationCompression, Fatal, TEXT("%i: unknown or unsupported rotation compression"), (int32)Seq.RotationCompressionFormat);
		};

		// setup Scale codec
		switch (Seq.ScaleCompressionFormat)
		{
		case ACF_None:
			Seq.ScaleCodec = &AEFConstantKeyLerp_None;
			break;
		case ACF_Float96NoW:
			Seq.ScaleCodec = &AEFConstantKeyLerp_Float96NoW;
			break;
		case ACF_IntervalFixed32NoW:
			Seq.ScaleCodec = &AEFConstantKeyLerp_IntervalFixed32NoW;
			break;
		case ACF_Identity:
			Seq.ScaleCodec = &AEFConstantKeyLerp_Identity;
			break;
		default:
			assert(false);// UE_LOG(LogAnimationCompression, Fatal, TEXT("%i: unknown or unsupported Scale compression"), (int32)Seq.ScaleCompressionFormat);
		};
	}
}

void AnimEncodingLegacyBase::GetBoneAtom(FTransform& OutAtom, const UAnimSequence& Seq, int32 TrackIndex, float Time)
{
	OutAtom.SetIdentity();

	// Use the CompressedTrackOffsets stream to find the data addresses
	const int32* __restrict TrackData = Seq.CompressedTrackOffsets.data() + (TrackIndex * 4);
	int32 TransKeysOffset = *(TrackData + 0);
	int32 NumTransKeys = *(TrackData + 1);
	int32 RotKeysOffset = *(TrackData + 2);
	int32 NumRotKeys = *(TrackData + 3);
	const uint8* __restrict TransStream = Seq.CompressedByteStream.data() + TransKeysOffset;
	const uint8* __restrict RotStream = Seq.CompressedByteStream.data() + RotKeysOffset;

	const float RelativePos = Time / (float)Seq.SequenceLength;

	// decompress the translation component using the proper method
	assert(Seq.TranslationCodec != NULL);
	((AnimEncodingLegacyBase*)Seq.TranslationCodec)->GetBoneAtomTranslation(OutAtom, Seq, TransStream, NumTransKeys, Time, RelativePos);

	// decompress the rotation component using the proper method
	assert(Seq.RotationCodec != NULL);
	((AnimEncodingLegacyBase*)Seq.RotationCodec)->GetBoneAtomRotation(OutAtom, Seq, RotStream, NumRotKeys, Time, RelativePos);

	// we assume scale keys can be empty, so only extrace if we have valid keys
	bool bHasValidScale = Seq.CompressedScaleOffsets.IsValid();
	if (bHasValidScale)
	{
		int32 ScaleKeyOffset = Seq.CompressedScaleOffsets.GetOffsetData(TrackIndex, 0);
		int32 NumScaleKeys = Seq.CompressedScaleOffsets.GetOffsetData(TrackIndex, 1);
		const uint8* __restrict ScaleStream = Seq.CompressedByteStream.data() + ScaleKeyOffset;
		// decompress the rotation component using the proper method
		assert(Seq.ScaleCodec != NULL);
		((AnimEncodingLegacyBase*)Seq.ScaleCodec)->GetBoneAtomScale(OutAtom, Seq, ScaleStream, NumScaleKeys, Time, RelativePos);
	}
}
