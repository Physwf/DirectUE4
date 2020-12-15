#pragma once

#include "BoneIndices.h"
#include "AnimTypes.h"
#include "AnimationAsset.h"
#include "AnimCurveTypes.h"
#include "BonePose.h"

class FAnimationRuntime
{
public:
	static void ExcludeBonesWithNoParents(const std::vector<int32>& BoneIndices, const FReferenceSkeleton& RefSkeleton, std::vector<int32>& FilteredRequiredBones);
	static void AccumulateAdditivePose(FCompactPose& BasePose, const FCompactPose& AdditivePose, FBlendedCurve& BaseCurve, const FBlendedCurve& AdditiveCurve, float Weight, enum EAdditiveAnimationType AdditiveType);
	static void RetargetBoneTransform(const USkeleton* MySkeleton, const std::string& RetargetSource, FTransform& BoneTransform, const int32 SkeletonBoneIndex, const FCompactPoseBoneIndex& BoneIndex, const FBoneContainer& RequiredBones, const bool bIsBakedAdditive);
};
