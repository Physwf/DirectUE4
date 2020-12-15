#include "AnimationRuntime.h"
#include "log.h"

void FAnimationRuntime::ExcludeBonesWithNoParents(const std::vector<int32>& BoneIndices, const FReferenceSkeleton& RefSkeleton, std::vector<int32>& FilteredRequiredBones)
{
	// Filter list, we only want bones that have their parents present in this array.
	FilteredRequiredBones.clear();

	for (uint32 Index = 0; Index < BoneIndices.size(); Index++)
	{
		const int32& BoneIndex = BoneIndices[Index];
		// Always add root bone.
		if (BoneIndex == 0)
		{
			FilteredRequiredBones.push_back(BoneIndex);
		}
		else
		{
			const int32 ParentBoneIndex = RefSkeleton.GetParentIndex(BoneIndex);
			if (std::find(FilteredRequiredBones.begin(), FilteredRequiredBones.end(), ParentBoneIndex) != FilteredRequiredBones.end())
			{
				FilteredRequiredBones.push_back(BoneIndex);
			}
			else
			{
				X_LOG("ExcludeBonesWithNoParents: Filtering out bone (%s) since parent (%s) is missing", RefSkeleton.GetBoneName(BoneIndex).c_str(), RefSkeleton.GetBoneName(ParentBoneIndex).c_str());
			}
		}
	}
}

void FAnimationRuntime::AccumulateAdditivePose(FCompactPose& BasePose, const FCompactPose& AdditivePose, FBlendedCurve& BaseCurve, const FBlendedCurve& AdditiveCurve, float Weight, enum EAdditiveAnimationType AdditiveType)
{

}

void FAnimationRuntime::RetargetBoneTransform(const USkeleton* MySkeleton, const std::string& RetargetSource, FTransform& BoneTransform, const int32 SkeletonBoneIndex, const FCompactPoseBoneIndex& BoneIndex, const FBoneContainer& RequiredBones, const bool bIsBakedAdditive)
{

}
