#include "ReferenceSkeleton.h"

void FReferenceSkeleton::EnsureParentsExist(std::vector<FBoneIndexType>& InOutBoneSortedArray) const
{
	/*
	const int32 NumBones = GetNum();
	// Iterate through existing array.
	int32 i = 0;

	TArray<bool>& BoneExists = FEnsureParentsExistScratchArea::Get().BoneExists;
	BoneExists.Reset();
	BoneExists.SetNumZeroed(NumBones);

	while (i < InOutBoneSortedArray.Num())
	{
		const int32 BoneIndex = InOutBoneSortedArray[i];

		// For the root bone, just move on.
		if (BoneIndex > 0)
		{
#if	!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			// Warn if we're getting bad data.
			// Bones are matched as int32, and a non found bone will be set to INDEX_NONE == -1
			// This should never happen, so if it does, something is wrong!
			if (BoneIndex >= NumBones)
			{
				UE_LOG(LogAnimation, Log, TEXT("FAnimationRuntime::EnsureParentsExist, BoneIndex >= RefSkeleton.GetNum()."));
				i++;
				continue;
			}
#endif
			BoneExists[BoneIndex] = true;

			const int32 ParentIndex = GetParentIndex(BoneIndex);

			// If we do not have this parent in the array, we add it in this location, and leave 'i' where it is.
			// This can happen if somebody removes bones in the physics asset, then it will try add back in, and in the process, 
			// parent can be missing
			if (!BoneExists[ParentIndex])
			{
				InOutBoneSortedArray.InsertUninitialized(i);
				InOutBoneSortedArray[i] = ParentIndex;
				BoneExists[ParentIndex] = true;
			}
			// If parent was in array, just move on.
			else
			{
				i++;
			}
		}
		else
		{
			BoneExists[0] = true;
			i++;
		}
	}
	*/
}

void FReferenceSkeleton::EnsureParentsExistAndSort(std::vector<FBoneIndexType>& InOutBoneUnsortedArray) const
{
	/*
	InOutBoneUnsortedArray.Sort();

	EnsureParentsExist(InOutBoneUnsortedArray);

	InOutBoneUnsortedArray.Sort();
	*/
}

