#pragma once

#include "AnimTypes.h"
#include "AnimSequence.h"



struct BoneTrackPair
{
	int32 AtomIndex;
	int32 TrackIndex;

	BoneTrackPair& operator=(const BoneTrackPair& Other)
	{
		this->AtomIndex = Other.AtomIndex;
		this->TrackIndex = Other.TrackIndex;
		return *this;
	}

	BoneTrackPair() {}
	BoneTrackPair(int32 Atom, int32 Track) :AtomIndex(Atom), TrackIndex(Track) {}
};

/**
*	Fixed-size array of BoneTrackPair elements.
*	Used in the bulk-animation solving process.
*/
#define MAX_BONES 65536 // DesiredBones is passed to the decompression routines as a TArray<FBoneIndexType>, so we know this max is appropriate
typedef std::vector<BoneTrackPair> BoneTrackArray;

typedef std::vector<FTransform> FTransformArray;

void AnimationFormat_GetBoneAtom(FTransform& OutAtom,
	const UAnimSequence& Seq,
	int32 TrackIndex,
	float Time);

void AnimationFormat_GetAnimationPose(
	FTransformArray& Atoms,
	const BoneTrackArray& RotationTracks,
	const BoneTrackArray& TranslationTracks,
	const BoneTrackArray& ScaleTracks,
	const UAnimSequence& Seq,
	float Time);

void AnimationFormat_SetInterfaceLinks(UAnimSequence& Seq);

extern const int32 CompressedTranslationStrides[ACF_MAX];
extern const int32 CompressedTranslationNum[ACF_MAX];
extern const int32 CompressedRotationStrides[ACF_MAX];
extern const int32 CompressedRotationNum[ACF_MAX];
extern const int32 CompressedScaleStrides[ACF_MAX];
extern const int32 CompressedScaleNum[ACF_MAX];
extern const uint8 PerTrackNumComponentTable[ACF_MAX * 8];

class AnimEncoding
{
public:
	virtual void GetBoneAtom(
		FTransform& OutAtom,
		const UAnimSequence& Seq,
		int32 TrackIndex,
		float Time) = 0;

	virtual void GetPoseRotations(
		FTransformArray& Atoms,
		const BoneTrackArray& DesiredPairs,
		const UAnimSequence& Seq,
		float Time) = 0;

	virtual void GetPoseTranslations(
		FTransformArray& Atoms,
		const BoneTrackArray& DesiredPairs,
		const UAnimSequence& Seq,
		float Time) = 0;

	virtual void GetPoseScales(
		FTransformArray& Atoms,
		const BoneTrackArray& DesiredPairs,
		const UAnimSequence& Seq,
		float Time) = 0;

protected:
	static float TimeToIndex(
		const UAnimSequence& Seq,
		float RelativePos,
		int32 NumKeys,
		int32 &PosIndex0Out,
		int32 &PosIndex1Out);

	static float TimeToIndex(
		const UAnimSequence& Seq,
		const uint8* FrameTable,
		float RelativePos,
		int32 NumKeys,
		int32 &PosIndex0Out,
		int32 &PosIndex1Out);
};

class AnimEncodingLegacyBase : public AnimEncoding
{
public:
	virtual void GetBoneAtom(
		FTransform& OutAtom,
		const UAnimSequence& Seq,
		int32 TrackIndex,
		float Time) override;

	virtual void GetBoneAtomRotation(
		FTransform& OutAtom,
		const UAnimSequence& Seq,
		const uint8* __restrict Stream,
		int32 NumKeys,
		float Time,
		float RelativePos) = 0;

	virtual void GetBoneAtomTranslation(
		FTransform& OutAtom,
		const UAnimSequence& Seq,
		const uint8* __restrict Stream,
		int32 NumKeys,
		float Time,
		float RelativePos) = 0;

	virtual void GetBoneAtomScale(
		FTransform& OutAtom,
		const UAnimSequence& Seq,
		const uint8* __restrict Stream,
		int32 NumKeys,
		float Time,
		float RelativePos) = 0;
};

inline float AnimEncoding::TimeToIndex(
	const UAnimSequence& Seq,
	float RelativePos,
	int32 NumKeys,
	int32 &PosIndex0Out,
	int32 &PosIndex1Out)
{
	const float SequenceLength = Seq.SequenceLength;
	float Alpha;

	if (NumKeys < 2)
	{
		assert(NumKeys == 1); // check if data is empty for some reason.
		PosIndex0Out = 0;
		PosIndex1Out = 0;
		return 0.0f;
	}
	// Check for before-first-frame case.
	if (RelativePos <= 0.f)
	{
		PosIndex0Out = 0;
		PosIndex1Out = 0;
		Alpha = 0.0f;
	}
	else
	{
		NumKeys -= 1; // never used without the minus one in this case
					  // Check for after-last-frame case.
		if (RelativePos >= 1.0f)
		{
			// If we're not looping, key n-1 is the final key.
			PosIndex0Out = NumKeys;
			PosIndex1Out = NumKeys;
			Alpha = 0.0f;
		}
		else
		{
			// For non-looping animation, the last frame is the ending frame, and has no duration.
			const float KeyPos = RelativePos * float(NumKeys);
			assert(KeyPos >= 0.0f);
			const float KeyPosFloor = floorf(KeyPos);
			PosIndex0Out = FMath::Min(FMath::TruncToInt(KeyPosFloor), NumKeys);
			Alpha = Seq.Interpolation == EAnimInterpolationType::Step ? 0.0f : KeyPos - KeyPosFloor;
			PosIndex1Out = FMath::Min(PosIndex0Out + 1, NumKeys);
		}
	}
	return Alpha;
}

template <typename TABLE_TYPE>
inline int32 FindLowKeyIndex(
	const TABLE_TYPE* FrameTable,
	int32 NumKeys,
	int32 SearchFrame,
	int32 KeyEstimate)
{
	const int32 LastKeyIndex = NumKeys - 1;
	int32 LowKeyIndex = KeyEstimate;

	if (FrameTable[KeyEstimate] <= SearchFrame)
	{
		// unless we find something better, we'll default to the last key
		LowKeyIndex = LastKeyIndex;

		// search forward from the estimate for the first value greater than our search parameter
		// if found, this is the high key and we want the one just prior to it
		for (int32 i = KeyEstimate + 1; i <= LastKeyIndex; ++i)
		{
			if (FrameTable[i] > SearchFrame)
			{
				LowKeyIndex = i - 1;
				break;
			}
		}
	}
	else
	{
		// unless we find something better, we'll default to the first key
		LowKeyIndex = 0;

		// search backward from the estimate for the first value less than or equal to the search parameter
		// if found, this is the low key we are searching for
		for (int32 i = KeyEstimate - 1; i > 0; --i)
		{
			if (FrameTable[i] <= SearchFrame)
			{
				LowKeyIndex = i;
				break;
			}
		}
	}

	return LowKeyIndex;
}

inline float AnimEncoding::TimeToIndex(
	const UAnimSequence& Seq,
	const uint8* FrameTable,
	float RelativePos,
	int32 NumKeys,
	int32 &PosIndex0Out,
	int32 &PosIndex1Out)
{
	float Alpha = 0.0f;

	assert(NumKeys != 0);

	const int32 LastKey = NumKeys - 1;

	int32 TotalFrames = Seq.NumFrames - 1;
	int32 EndingKey = LastKey;

	if (NumKeys < 2 || RelativePos <= 0.f)
	{
		// return the first key
		PosIndex0Out = 0;
		PosIndex1Out = 0;
		Alpha = 0.0f;
	}
	else if (RelativePos >= 1.0f)
	{
		// return the ending key
		PosIndex0Out = EndingKey;
		PosIndex1Out = EndingKey;
		Alpha = 0.0f;
	}
	else
	{
		// find the proper key range to return
		const int32 LastFrame = TotalFrames - 1;
		const float KeyPos = RelativePos * (float)LastKey;
		const float FramePos = RelativePos * (float)TotalFrames;
		const int32 FramePosFloor = FMath::Clamp(FMath::TruncToInt(FramePos), 0, LastFrame);
		const int32 KeyEstimate = FMath::Clamp(FMath::TruncToInt(KeyPos), 0, LastKey);

		int32 LowFrame = 0;
		int32 HighFrame = 0;

		// find the pair of keys which surround our target frame index
		if (Seq.NumFrames > 0xFF)
		{
			const uint16* Frames = (uint16*)FrameTable;
			PosIndex0Out = FindLowKeyIndex<uint16>(Frames, NumKeys, FramePosFloor, KeyEstimate);
			LowFrame = Frames[PosIndex0Out];

			PosIndex1Out = PosIndex0Out + 1;
			if (PosIndex1Out > LastKey)
			{
				PosIndex1Out = EndingKey;
			}
			HighFrame = Frames[PosIndex1Out];
		}
		else
		{
			const uint8* Frames = (uint8*)FrameTable;
			PosIndex0Out = FindLowKeyIndex<uint8>(Frames, NumKeys, FramePosFloor, KeyEstimate);
			LowFrame = Frames[PosIndex0Out];

			PosIndex1Out = PosIndex0Out + 1;
			if (PosIndex1Out > LastKey)
			{
				PosIndex1Out = EndingKey;
			}
			HighFrame = Frames[PosIndex1Out];
		}

		// compute the blend parameters for the keys we have found
		int32 Delta = FMath::Max(HighFrame - LowFrame, 1);
		const float Remainder = (FramePos - (float)LowFrame);
		Alpha = Seq.Interpolation == EAnimInterpolationType::Step ? 0.f : (Remainder / (float)Delta);
	}

	return Alpha;
}