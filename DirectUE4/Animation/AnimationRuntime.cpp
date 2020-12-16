#include "AnimationRuntime.h"
#include "log.h"

enum ETypeAdvanceAnim FAnimationRuntime::AdvanceTime(const bool& bAllowLooping, const float& MoveDelta, float& InOutTime, const float& EndTime)
{
	InOutTime += MoveDelta;

	if (InOutTime < 0.f || InOutTime > EndTime)
	{
		if (bAllowLooping)
		{
			if (EndTime != 0.f)
			{
				InOutTime = FMath::Fmod(InOutTime, EndTime);
				// Fmod doesn't give result that falls into (0, EndTime), but one that falls into (-EndTime, EndTime). Negative values need to be handled in custom way
				if (InOutTime < 0.f)
				{
					InOutTime += EndTime;
				}
			}
			else
			{
				// end time is 0.f
				InOutTime = 0.f;
			}

			// it has been looped
			return ETAA_Looped;
		}
		else
		{
			// If not, snap time to end of sequence and stop playing.
			InOutTime = FMath::Clamp(InOutTime, 0.f, EndTime);
			return ETAA_Finished;
		}
	}

	return ETAA_Default;
}

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
