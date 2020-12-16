#include "AnimEncoding.h"

class AEFConstantKeyLerpShared : public AnimEncodingLegacyBase
{
public:
};

template<int32 FORMAT>
class AEFConstantKeyLerp : public AEFConstantKeyLerpShared
{
public:
	void GetBoneAtomRotation(
		FTransform& OutAtom,
		const UAnimSequence& Seq,
		const uint8* __restrict Stream,
		int32 NumKeys,
		float Time,
		float RelativePos);

	void GetBoneAtomTranslation(
		FTransform& OutAtom,
		const UAnimSequence& Seq,
		const uint8* __restrict Stream,
		int32 NumKeys,
		float Time,
		float RelativePos);

	void GetBoneAtomScale(
		FTransform& OutAtom,
		const UAnimSequence& Seq,
		const uint8* __restrict Stream,
		int32 NumKeys,
		float Time,
		float RelativePos);

	void GetPoseRotations(
		FTransformArray& Atoms,
		const BoneTrackArray& DesiredPairs,
		const UAnimSequence& Seq,
		float RelativePos);

	void GetPoseTranslations(
		FTransformArray& Atoms,
		const BoneTrackArray& DesiredPairs,
		const UAnimSequence& Seq,
		float RelativePos);

	void GetPoseScales(
		FTransformArray& Atoms,
		const BoneTrackArray& DesiredPairs,
		const UAnimSequence& Seq,
		float RelativePos);
};

template<int32 FORMAT>
inline void AEFConstantKeyLerp<FORMAT>::GetBoneAtomRotation(
	FTransform& OutAtom,
	const UAnimSequence& Seq,
	const uint8* __restrict RotStream,
	int32 NumRotKeys,
	float Time,
	float RelativePos)
{
	if (NumRotKeys == 1)
	{
		// For a rotation track of n=1 keys, the single key is packed as an FQuatFloat96NoW.
		FQuat R0;
		DecompressRotation<ACF_Float96NoW>(R0, RotStream, RotStream);
		OutAtom.SetRotation(R0);
	}
	else
	{
		int32 Index0;
		int32 Index1;
		float Alpha = TimeToIndex(Seq, RelativePos, NumRotKeys, Index0, Index1);

		const int32 RotationStreamOffset = (FORMAT == ACF_IntervalFixed32NoW) ? (sizeof(float) * 6) : 0; // offset past Min and Range data

		if (Index0 != Index1)
		{

			// unpack and lerp between the two nearest keys
			const uint8* __restrict KeyData0 = RotStream + RotationStreamOffset + (Index0*CompressedRotationStrides[FORMAT] * CompressedRotationNum[FORMAT]);
			const uint8* __restrict KeyData1 = RotStream + RotationStreamOffset + (Index1*CompressedRotationStrides[FORMAT] * CompressedRotationNum[FORMAT]);
			FQuat R0;
			FQuat R1;
			DecompressRotation<FORMAT>(R0, RotStream, KeyData0);
			DecompressRotation<FORMAT>(R1, RotStream, KeyData1);

			// Fast linear quaternion interpolation.
			FQuat BlendedQuat = FQuat::FastLerp(R0, R1, Alpha);
			BlendedQuat.Normalize();
			OutAtom.SetRotation(BlendedQuat);
		}
		else // (Index0 == Index1)
		{

			// unpack a single key
			const uint8* __restrict KeyData = RotStream + RotationStreamOffset + (Index0*CompressedRotationStrides[FORMAT] * CompressedRotationNum[FORMAT]);
			FQuat R0;
			DecompressRotation<FORMAT>(R0, RotStream, KeyData);
			OutAtom.SetRotation(R0);
		}
	}
}

template<int32 FORMAT>
inline void AEFConstantKeyLerp<FORMAT>::GetBoneAtomTranslation(
	FTransform& OutAtom,
	const UAnimSequence& Seq,
	const uint8* __restrict TransStream,
	int32 NumTransKeys,
	float Time,
	float RelativePos)
{
	int32 Index0;
	int32 Index1;
	float Alpha = TimeToIndex(Seq, RelativePos, NumTransKeys, Index0, Index1);

	const int32 TransStreamOffset = ((FORMAT == ACF_IntervalFixed32NoW) && NumTransKeys > 1) ? (sizeof(float) * 6) : 0; // offset past Min and Range data

	if (Index0 != Index1)
	{
		const uint8* __restrict KeyData0 = TransStream + TransStreamOffset + Index0 * CompressedTranslationStrides[FORMAT] * CompressedTranslationNum[FORMAT];
		const uint8* __restrict KeyData1 = TransStream + TransStreamOffset + Index1 * CompressedTranslationStrides[FORMAT] * CompressedTranslationNum[FORMAT];
		FVector P0;
		FVector P1;
		DecompressTranslation<FORMAT>(P0, TransStream, KeyData0);
		DecompressTranslation<FORMAT>(P1, TransStream, KeyData1);
		OutAtom.SetTranslation(FMath::Lerp(P0, P1, Alpha));
	}
	else // (Index0 == Index1)
	{
		// unpack a single key
		const uint8* __restrict KeyData = TransStream + TransStreamOffset + Index0 * CompressedTranslationStrides[FORMAT] * CompressedTranslationNum[FORMAT];
		FVector P0;
		DecompressTranslation<FORMAT>(P0, TransStream, KeyData);
		OutAtom.SetTranslation(P0);
	}
}

template<int32 FORMAT>
inline void AEFConstantKeyLerp<FORMAT>::GetBoneAtomScale(
	FTransform& OutAtom,
	const UAnimSequence& Seq,
	const uint8* __restrict ScaleStream,
	int32 NumScaleKeys,
	float Time,
	float RelativePos)
{
	int32 Index0;
	int32 Index1;
	float Alpha = TimeToIndex(Seq, RelativePos, NumScaleKeys, Index0, Index1);

	const int32 ScaleStreamOffset = ((FORMAT == ACF_IntervalFixed32NoW) && NumScaleKeys > 1) ? (sizeof(float) * 6) : 0; // offset past Min and Range data

	if (Index0 != Index1)
	{
		const uint8* __restrict KeyData0 = ScaleStream + ScaleStreamOffset + Index0 * CompressedScaleStrides[FORMAT] * CompressedScaleNum[FORMAT];
		const uint8* __restrict KeyData1 = ScaleStream + ScaleStreamOffset + Index1 * CompressedScaleStrides[FORMAT] * CompressedScaleNum[FORMAT];
		FVector P0;
		FVector P1;
		DecompressScale<FORMAT>(P0, ScaleStream, KeyData0);
		DecompressScale<FORMAT>(P1, ScaleStream, KeyData1);
		OutAtom.SetScale3D(FMath::Lerp(P0, P1, Alpha));
	}
	else // (Index0 == Index1)
	{
		// unpack a single key
		const uint8* __restrict KeyData = ScaleStream + ScaleStreamOffset + Index0 * CompressedScaleStrides[FORMAT] * CompressedScaleNum[FORMAT];
		FVector P0;
		DecompressScale<FORMAT>(P0, ScaleStream, KeyData);
		OutAtom.SetScale3D(P0);
	}
}

template<int32 FORMAT>
inline void AEFConstantKeyLerp<FORMAT>::GetPoseRotations(
	FTransformArray& Atoms,
	const BoneTrackArray& DesiredPairs,
	const UAnimSequence& Seq,
	float Time)
{
	const int32 PairCount = (int32)DesiredPairs.size();
	const float RelativePos = Time / (float)Seq.SequenceLength;

	for (int32 PairIndex = 0; PairIndex < PairCount; ++PairIndex)
	{
		const BoneTrackPair& Pair = DesiredPairs[PairIndex];
		const int32 TrackIndex = Pair.TrackIndex;
		const int32 AtomIndex = Pair.AtomIndex;
		FTransform& BoneAtom = Atoms[AtomIndex];

		const int32* __restrict TrackData = Seq.CompressedTrackOffsets.data() + (TrackIndex * 4);
		const int32 RotKeysOffset = *(TrackData + 2);
		const int32 NumRotKeys = *(TrackData + 3);
		const uint8* __restrict RotStream = Seq.CompressedByteStream.data() + RotKeysOffset;

		// call the decoder directly (not through the vtable)
		AEFConstantKeyLerp<FORMAT>::GetBoneAtomRotation(BoneAtom, Seq, RotStream, NumRotKeys, Time, RelativePos);
	}
}

template<int32 FORMAT>
inline void AEFConstantKeyLerp<FORMAT>::GetPoseTranslations(
	FTransformArray& Atoms,
	const BoneTrackArray& DesiredPairs,
	const UAnimSequence& Seq,
	float Time)
{
	const int32 PairCount = (int32)DesiredPairs.size();
	const float RelativePos = Time / (float)Seq.SequenceLength;

	//@TODO: Verify that this prefetch is helping
	// Prefetch the desired pairs array and 2 destination spots; the loop will prefetch one 2 out each iteration
	//FPlatformMisc::Prefetch(&(DesiredPairs[0]));
	const int32 PrefetchCount = FMath::Min(PairCount, 1);
	for (int32 PairIndex = 0; PairIndex < PairCount; ++PairIndex)
	{
		const BoneTrackPair& Pair = DesiredPairs[PairIndex];
		//FPlatformMisc::Prefetch(Atoms.GetData() + Pair.AtomIndex);
	}

	for (int32 PairIndex = 0; PairIndex < PairCount; ++PairIndex)
	{
		int32 PrefetchIndex = PairIndex + PrefetchCount;
		if (PrefetchIndex < PairCount)
		{
			//FPlatformMisc::Prefetch(Atoms.GetData() + DesiredPairs[PrefetchIndex].AtomIndex);
		}

		const BoneTrackPair& Pair = DesiredPairs[PairIndex];
		const int32 TrackIndex = Pair.TrackIndex;
		const int32 AtomIndex = Pair.AtomIndex;
		FTransform& BoneAtom = Atoms[AtomIndex];

		const int32* __restrict TrackData = Seq.CompressedTrackOffsets.data() + (TrackIndex * 4);
		const int32 TransKeysOffset = *(TrackData + 0);
		const int32 NumTransKeys = *(TrackData + 1);
		const uint8* __restrict TransStream = Seq.CompressedByteStream.data() + TransKeysOffset;

		// call the decoder directly (not through the vtable)
		AEFConstantKeyLerp<FORMAT>::GetBoneAtomTranslation(BoneAtom, Seq, TransStream, NumTransKeys, Time, RelativePos);
	}
}

template<int32 FORMAT>
inline void AEFConstantKeyLerp<FORMAT>::GetPoseScales(
	FTransformArray& Atoms,
	const BoneTrackArray& DesiredPairs,
	const UAnimSequence& Seq,
	float Time)
{
	assert(Seq.CompressedScaleOffsets.IsValid());

	const int32 PairCount = (int32)DesiredPairs.size();
	const float RelativePos = Time / (float)Seq.SequenceLength;

	//@TODO: Verify that this prefetch is helping
	// Prefetch the desired pairs array and 2 destination spots; the loop will prefetch one 2 out each iteration
	//FPlatformMisc::Prefetch(&(DesiredPairs[0]));
	const int32 PrefetchCount = FMath::Min(PairCount, 1);
	for (int32 PairIndex = 0; PairIndex < PairCount; ++PairIndex)
	{
		const BoneTrackPair& Pair = DesiredPairs[PairIndex];
		//FPlatformMisc::Prefetch(Atoms.GetData() + Pair.AtomIndex);
	}

	for (int32 PairIndex = 0; PairIndex < PairCount; ++PairIndex)
	{
		int32 PrefetchIndex = PairIndex + PrefetchCount;
		if (PrefetchIndex < PairCount)
		{
			//FPlatformMisc::Prefetch(Atoms.GetData() + DesiredPairs[PrefetchIndex].AtomIndex);
		}

		const BoneTrackPair& Pair = DesiredPairs[PairIndex];
		const int32 TrackIndex = Pair.TrackIndex;
		const int32 AtomIndex = Pair.AtomIndex;
		FTransform& BoneAtom = Atoms[AtomIndex];

		const int32 ScaleKeysOffset = Seq.CompressedScaleOffsets.GetOffsetData(TrackIndex, 0);
		const int32 NumScaleKeys = Seq.CompressedScaleOffsets.GetOffsetData(TrackIndex, 1);
		const uint8* __restrict ScaleStream = Seq.CompressedByteStream.data() + ScaleKeysOffset;

		// call the decoder directly (not through the vtable)
		AEFConstantKeyLerp<FORMAT>::GetBoneAtomScale(BoneAtom, Seq, ScaleStream, NumScaleKeys, Time, RelativePos);
	}
}