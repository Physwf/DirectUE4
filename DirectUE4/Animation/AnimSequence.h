#pragma once

#include "AnimTypes.h"
#include "AnimationAsset.h"
#include "AnimCurveTypes.h"
#include "AnimSequenceBase.h"
#include "BoneIndices.h"
#include "AnimEnums.h"

#include <vector>
#include <string>

struct FAnimCompressContext;

enum AnimationKeyFormat
{
	AKF_ConstantKeyLerp,
	AKF_VariableKeyLerp,
	AKF_PerTrackCompression,
	AKF_MAX,
};


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

struct FCompressedOffsetData
{
	std::vector<int32> OffsetData;

	int32 StripSize;

	FCompressedOffsetData(int32 InStripSize = 2)
		: StripSize(InStripSize)
	{}

	void SetStripSize(int32 InStripSize)
	{
		assert(InStripSize > 0);
		StripSize = InStripSize;
	}

	const int32 GetOffsetData(int32 StripIndex, int32 Offset) const
	{
		assert(IsValidIndex(OffsetData, StripIndex * StripSize + Offset));

		return OffsetData[StripIndex * StripSize + Offset];
	}

	void SetOffsetData(int32 StripIndex, int32 Offset, int32 Value)
	{
		assert(IsValidIndex(OffsetData, StripIndex * StripSize + Offset));
		OffsetData[StripIndex * StripSize + Offset] = Value;
	}

	void AddUninitialized(int32 NumOfTracks)
	{
		OffsetData.resize(NumOfTracks*StripSize);
	}

	void Empty(int32 NumOfTracks = 0)
	{
		OffsetData.clear();
		OffsetData.reserve(NumOfTracks*StripSize);
	}

	int32 GetMemorySize() const
	{
		return sizeof(int32)*OffsetData.size() + sizeof(int32);
	}

	int32 GetNumTracks() const
	{
		return OffsetData.size() / StripSize;
	}

	bool IsValid() const
	{
		return (OffsetData.size() > 0);
	}
};

class UAnimSequence : public UAnimSequenceBase
{
public:
	int32 NumFrames;
protected:
	std::vector<struct FTrackToSkeletonMap> TrackToSkeletonMapTable;
	std::vector<struct FRawAnimSequenceTrack> RawAnimationData;
	std::vector<std::string> AnimationTrackNames;
	std::vector<struct FRawAnimSequenceTrack> SourceRawAnimationData;
	std::vector<struct FTrackToSkeletonMap> CompressedTrackToSkeletonMapTable;
	std::vector<std::string>				UniqueMarkerNames;
public:
	class UAssetImportData* AssetImportData;
	class UAnimSequence* RefPoseSeq;
	int32 RefFrameIndex;
	EAdditiveAnimationType AdditiveAnimType;
	EAdditiveBasePoseType RefPoseType;
	bool bEnableRootMotion;
	std::string RetargetSource;

	EAnimInterpolationType Interpolation;

	AnimationCompressionFormat TranslationCompressionFormat;
	/** The compression format that was used to compress rotation tracks. */
	AnimationCompressionFormat RotationCompressionFormat;

	/** The compression format that was used to compress rotation tracks. */
	AnimationCompressionFormat ScaleCompressionFormat;
	/**
	* An array of 4*NumTrack ints, arranged as follows: - PerTrack is 2*NumTrack, so this isn't true any more
	*   [0] Trans0.Offset
	*   [1] Trans0.NumKeys
	*   [2] Rot0.Offset
	*   [3] Rot0.NumKeys
	*   [4] Trans1.Offset
	*   . . .
	*/
	std::vector<int32> CompressedTrackOffsets;

	/**
	* An array of 2*NumTrack ints, arranged as follows:
	if identity, it is offset
	if not, it is num of keys
	*   [0] Scale0.Offset or NumKeys
	*   [1] Scale1.Offset or NumKeys

	* @TODO NOTE: first implementation is offset is [0], numkeys [1]
	*   . . .
	*/
	FCompressedOffsetData CompressedScaleOffsets;

	/**
	* ByteStream for compressed animation data.
	* All keys are currently stored at evenly-spaced intervals (ie no explicit key times).
	*
	* For a translation track of n keys, data is packed as n uncompressed float[3]:
	*
	* For a rotation track of n>1 keys, the first 24 bytes are reserved for compression info
	* (eg Fixed32 stores float Mins[3]; float Ranges[3]), followed by n elements of the compressed type.
	* For a rotation track of n=1 keys, the single key is packed as an FQuatFloat96NoW.
	*/
	std::vector<uint8> CompressedByteStream;

	AnimationKeyFormat KeyEncodingFormat;

	class AnimEncoding* TranslationCodec;

	class AnimEncoding* RotationCodec;

	class AnimEncoding* ScaleCodec;

	void  UpdateCompressedTrackMapFromRaw() { CompressedTrackToSkeletonMapTable = TrackToSkeletonMapTable; }
	int32 GetSkeletonIndexFromCompressedDataTrackIndex(const int32 TrackIndex) const
	{
		return CompressedTrackToSkeletonMapTable[TrackIndex].BoneTreeIndex;
	}
	const std::vector<FRawAnimSequenceTrack>& GetRawAnimationData() const { return RawAnimationData; }

	bool OnlyUseRawData() const { return bUseRawDataOnly; }
	void SetUseRawDataOnly(bool bInUseRawDataOnly) { bUseRawDataOnly = bInUseRawDataOnly; }

	void ResizeSequence(float NewLength, int32 NewNumFrames, bool bInsert, int32 StartFrame/*inclusive */, int32 EndFrame/*inclusive*/);
	virtual int32 GetNumberOfFrames() const override { return NumFrames; }

	void PostProcessSequence(bool bForceNewRawDatGuid = true);
	bool CompressRawAnimData();
	bool CompressRawAnimData(float MaxPosDiff, float MaxAngleDiff);
	bool CompressRawAnimSequenceTrack(FRawAnimSequenceTrack& RawTrack, float MaxPosDiff, float MaxAngleDiff);
	void OnRawDataChanged();

	void RequestAnimCompression(bool bAsyncCompression, bool AllowAlternateCompressor = false, bool bOutput = false);
	void RequestAnimCompression(bool bAsyncCompression, std::shared_ptr<FAnimCompressContext> CompressContext);
	void RequestSyncAnimRecompression(bool bOutput = false) { RequestAnimCompression(false, false, bOutput); }
	bool IsCompressedDataValid() const;

	int32 AddNewRawTrack(std::string TrackName, FRawAnimSequenceTrack* TrackData = nullptr);

	virtual std::vector<std::string>* GetUniqueMarkerNames() { return &UniqueMarkerNames; }
public:
	void CleanAnimSequenceForImport();
	bool HasSourceRawData() const { return SourceRawAnimationData.size() > 0; }

	virtual void GetAnimationPose(FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const override;
	void GetBonePose(FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext, bool bForceUseRawData = false) const;

	void GetBonePose_Additive(FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const;

	virtual bool IsValidAdditive() const override;
	virtual void EvaluateCurveData(FBlendedCurve& OutCurve, float CurrentTime, bool bForceUseRawData = false) const;
	void GetAdditiveBasePose(FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const;
private:
	bool bUseRawDataOnly;

	void RetargetBoneTransform(FTransform& BoneTransform, const int32 SkeletonBoneIndex, const FCompactPoseBoneIndex& BoneIndex, const FBoneContainer& RequiredBones, const bool bIsBakedAdditive) const;
	void GetBonePose_AdditiveMeshRotationOnly(FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const;

	bool UseRawDataForPoseExtraction(const FBoneContainer& RequiredBones) const;
};
