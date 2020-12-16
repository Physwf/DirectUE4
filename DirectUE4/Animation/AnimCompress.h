#pragma once

#include "AnimationUtils.h"

class FCompressionMemorySummary
{
public:
	FCompressionMemorySummary(bool bInEnabled);

	void GatherPreCompressionStats(UAnimSequence* Seq, int32 ProgressNumerator, int32 ProgressDenominator);

	void GatherPostCompressionStats(UAnimSequence* Seq, std::vector<FBoneData>& BoneData);

	~FCompressionMemorySummary();

private:
	bool bEnabled;
	bool bUsed;
	int32 TotalRaw;
	int32 TotalBeforeCompressed;
	int32 TotalAfterCompressed;

	float ErrorTotal;
	float ErrorCount;
	float AverageError;
	float MaxError;
	float MaxErrorTime;
	int32 MaxErrorBone;
	std::string MaxErrorBoneName;
	std::string MaxErrorAnimName;
};

//////////////////////////////////////////////////////////////////////////
// FAnimCompressContext - Context information / storage for use during
// animation compression
struct FAnimCompressContext
{
	FCompressionMemorySummary	CompressionSummary;
	uint32						AnimIndex;
	uint32						MaxAnimations;
	bool						bAllowAlternateCompressor;
	bool						bOutput;

	FAnimCompressContext(bool bInAllowAlternateCompressor, bool bInOutput, uint32 InMaxAnimations = 1) : CompressionSummary(bInOutput), AnimIndex(0), MaxAnimations(InMaxAnimations), bAllowAlternateCompressor(bInAllowAlternateCompressor), bOutput(bInOutput) {}

	void GatherPreCompressionStats(UAnimSequence* Seq);

	void GatherPostCompressionStats(UAnimSequence* Seq, std::vector<FBoneData>& BoneData);
};

class UAnimCompress
{
public:
	uint32 bNeedsSkeleton : 1;

	AnimationCompressionFormat TranslationCompressionFormat;
	/** Format for bitwise compression of rotation data. */
	AnimationCompressionFormat RotationCompressionFormat;
	/** Format for bitwise compression of scale data. */
	AnimationCompressionFormat ScaleCompressionFormat;

	bool Reduce(class UAnimSequence* AnimSeq, bool bOutput);
	bool Reduce(class UAnimSequence* AnimSeq, FAnimCompressContext& Context);
protected:
	virtual void DoReduction(class UAnimSequence* AnimSeq, const std::vector<class FBoneData>& BoneData) = 0;

	static void FilterTrivialPositionKeys(
		std::vector<struct FTranslationTrack>& Track,
		float MaxPosDelta);
	static void FilterTrivialPositionKeys(
		struct FTranslationTrack& Track,
		float MaxPosDelta);
	static void FilterTrivialRotationKeys(
		std::vector<struct FRotationTrack>& InputTracks,
		float MaxRotDelta);
	static void FilterTrivialRotationKeys(
		struct FRotationTrack& Track,
		float MaxRotDelta);
	static void FilterTrivialScaleKeys(
		std::vector<struct FScaleTrack>& Track,
		float MaxScaleDelta);
	static void FilterTrivialScaleKeys(
		struct FScaleTrack& Track,
		float MaxScaleDelta);
	static void SeparateRawDataIntoTracks(
		const std::vector<struct FRawAnimSequenceTrack>& RawAnimData,
		float SequenceLength,
		std::vector<struct FTranslationTrack>& OutTranslationData,
		std::vector<struct FRotationTrack>& OutRotationData,
		std::vector<struct FScaleTrack>& OutScaleData);

	static void FilterAnimRotationOnlyKeys(std::vector<FTranslationTrack>& PositionTracks, UAnimSequence* AnimSeq);

	static void FilterTrivialKeys(
		std::vector<struct FTranslationTrack>& PositionTracks,
		std::vector<struct FRotationTrack>& RotationTracks,
		std::vector<struct FScaleTrack>& ScaleTracks,
		float MaxPosDelta,
		float MaxRotDelta,
		float MaxScaleDelta);
	
	static void BitwiseCompressAnimationTracks(
		class UAnimSequence* Seq,
		AnimationCompressionFormat TargetTranslationFormat,
		AnimationCompressionFormat TargetRotationFormat,
		AnimationCompressionFormat TargetScaleFormat,
		const std::vector<FTranslationTrack>& TranslationData,
		const std::vector<FRotationTrack>& RotationData,
		const std::vector<FScaleTrack>& ScaleData,
		bool IncludeKeyTable = false);
};

class UAnimCompress_BitwiseCompressOnly : public UAnimCompress
{
protected:
	virtual void DoReduction(class UAnimSequence* AnimSeq, const std::vector<class FBoneData>& BoneData) override;
};
