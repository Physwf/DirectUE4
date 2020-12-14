#pragma once

#include "AnimSequenceBase.h"

#include <vector>
#include <string>

struct FRawAnimSequenceTrack
{
	/** Position keys. */
	std::vector<FVector> PosKeys;
	/** Rotation keys. */
	std::vector<FQuat> RotKeys;
	/** Scale keys. */
	std::vector<FVector> ScaleKeys;
};

struct FTrackToSkeletonMap
{
	// Index of Skeleton.BoneTree this Track belongs to.
	int32 BoneTreeIndex;
	FTrackToSkeletonMap()
		: BoneTreeIndex(0)
	{
	}
	FTrackToSkeletonMap(int32 InBoneTreeIndex)
		: BoneTreeIndex(InBoneTreeIndex)
	{
	}
};
struct FTranslationTrack
{
	std::vector<FVector> PosKeys;
	std::vector<float> Times;
};
/**
* Keyframe rotation data for one track.  Rot(i) occurs at Time(i).  Rot.Num() always equals Time.Num().
*/
struct FRotationTrack
{
	std::vector<FQuat> RotKeys;
	std::vector<float> Times;
};
/**
* Keyframe scale data for one track.  Scale(i) occurs at Time(i).  Rot.Num() always equals Time.Num().
*/
struct FScaleTrack
{
	std::vector<FVector> ScaleKeys;
	std::vector<float> Times;
};
class UAnimSequence : public UAnimSequenceBase
{
protected:
	std::vector<struct FTrackToSkeletonMap> TrackToSkeletonMapTable;
	std::vector<struct FRawAnimSequenceTrack> RawAnimationData;
	std::vector<std::string> AnimationTrackNames;
	std::vector<struct FRawAnimSequenceTrack> SourceRawAnimationData;
public:
	class UAssetImportData* AssetImportData;
public:
	void CleanAnimSequenceForImport();
	bool  HasSourceRawData() const { return SourceRawAnimationData.size() > 0; }
};
