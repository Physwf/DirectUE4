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