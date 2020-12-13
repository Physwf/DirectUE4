#include "ReferenceSkeleton.h"
#include "Skeleton.h"
#include "log.h"

#include <algorithm>

FTransform GetComponentSpaceTransform(std::vector<uint8>& ComponentSpaceFlags, std::vector<FTransform>& ComponentSpaceTransforms, FReferenceSkeleton& RefSkeleton, int32 TargetIndex)
{
	FTransform& This = ComponentSpaceTransforms[TargetIndex];

	if (!ComponentSpaceFlags[TargetIndex])
	{
		const int32 ParentIndex = RefSkeleton.GetParentIndex(TargetIndex);
		This *= GetComponentSpaceTransform(ComponentSpaceFlags, ComponentSpaceTransforms, RefSkeleton, ParentIndex);
		ComponentSpaceFlags[TargetIndex] = 1;
	}
	return This;
}


FReferenceSkeletonModifier::~FReferenceSkeletonModifier()
{
	RefSkeleton.RebuildRefSkeleton(Skeleton, true);
}

void FReferenceSkeletonModifier::UpdateRefPoseTransform(const int32 BoneIndex, const FTransform& BonePose)
{
	RefSkeleton.UpdateRefPoseTransform(BoneIndex, BonePose);
}

void FReferenceSkeletonModifier::Add(const FMeshBoneInfo& BoneInfo, const FTransform& BonePose)
{
	RefSkeleton.Add(BoneInfo, BonePose);
}

int32 FReferenceSkeletonModifier::FindBoneIndex(const std::string& BoneName) const
{
	return RefSkeleton.FindRawBoneIndex(BoneName);
}

const std::vector<FMeshBoneInfo> & FReferenceSkeletonModifier::GetRefBoneInfo() const
{
	return RefSkeleton.GetRawRefBoneInfo();
}

int32 FReferenceSkeleton::GetRawSourceBoneIndex(const USkeleton* Skeleton, const std::string& SourceBoneName) const
{
	for (const FVirtualBone& VB : Skeleton->GetVirtualBones())
	{
		//Is our source another virtual bone
		if (VB.VirtualBoneName == SourceBoneName)
		{
			//return our source virtual bones target, it is the same transform
			//but it exists in the raw bone array
			return FindBoneIndex(VB.TargetBoneName);
		}
	}
	return FindBoneIndex(SourceBoneName);
}

void FReferenceSkeleton::RebuildRefSkeleton(const USkeleton* Skeleton, bool bRebuildNameMap)
{
	if (bRebuildNameMap)
	{
		//On loading FinalRefBone data wont exist but NameToIndexMap will and will be valid
		RebuildNameToIndexMap();
	}

	const uint32 NumVirtualBones = Skeleton ? Skeleton->GetVirtualBones().size() : 0;
	FinalRefBoneInfo = std::vector<FMeshBoneInfo>(RawRefBoneInfo/*, NumVirtualBones*/);
	FinalRefBonePose = std::vector<FTransform>(RawRefBonePose/*, NumVirtualBones*/);
	FinalNameToIndexMap = RawNameToIndexMap;

	RequiredVirtualBones.clear();
	UsedVirtualBoneData.clear();

	if (NumVirtualBones > 0)
	{
		std::vector<uint8> ComponentSpaceFlags;
		ComponentSpaceFlags.resize(RawRefBonePose.size());
		ComponentSpaceFlags[0] = 1;

		std::vector<FTransform> ComponentSpaceTransforms = std::vector<FTransform>(RawRefBonePose);

		for (uint32 VirtualBoneIdx = 0; VirtualBoneIdx < NumVirtualBones; ++VirtualBoneIdx)
		{
			const uint32 ActualIndex = VirtualBoneIdx + RawRefBoneInfo.size();
			const FVirtualBone& VB = Skeleton->GetVirtualBones()[VirtualBoneIdx];

			const int32 SourceIndex = GetRawSourceBoneIndex(Skeleton, VB.SourceBoneName);
			const int32 ParentIndex = FindBoneIndex(VB.SourceBoneName);
			const int32 TargetIndex = FindBoneIndex(VB.TargetBoneName);
			if (ParentIndex != -1 && TargetIndex != -1)
			{
				FinalRefBoneInfo.push_back(FMeshBoneInfo(VB.VirtualBoneName, VB.VirtualBoneName, ParentIndex));

				const FTransform TargetCS = GetComponentSpaceTransform(ComponentSpaceFlags, ComponentSpaceTransforms, *this, TargetIndex);
				const FTransform SourceCS = GetComponentSpaceTransform(ComponentSpaceFlags, ComponentSpaceTransforms, *this, SourceIndex);

				FTransform VBTransform = TargetCS.GetRelativeTransform(SourceCS);

				const int32 NewBoneIndex = (int32)FinalRefBonePose.size();
				FinalRefBonePose.push_back(VBTransform);
				FinalNameToIndexMap[VB.VirtualBoneName] = NewBoneIndex;
				RequiredVirtualBones.push_back(NewBoneIndex);
				UsedVirtualBoneData.push_back(FVirtualBoneRefData(NewBoneIndex, SourceIndex, TargetIndex));
			}
		}
	}
}

void FReferenceSkeleton::RebuildNameToIndexMap()
{
	// Start by clearing the current map.
	RawNameToIndexMap.clear();

	// Then iterate over each bone, adding the name and bone index.
	const uint32 NumBones = RawRefBoneInfo.size();
	for (uint32 BoneIndex = 0; BoneIndex < NumBones; BoneIndex++)
	{
		const std::string& BoneName = RawRefBoneInfo[BoneIndex].Name;
		if (BoneName != "")
		{
			RawNameToIndexMap.insert(std::make_pair(BoneName, BoneIndex));
		}
		else
		{
			X_LOG("RebuildNameToIndexMap: Bone with no name detected for index: %d", BoneIndex);
		}
	}

}
struct FEnsureParentsExistScratchArea 
{
	static FEnsureParentsExistScratchArea& Get()
	{
		static FEnsureParentsExistScratchArea Instance;
		return Instance;
	}

	std::vector<bool> BoneExists;
};

void FReferenceSkeleton::EnsureParentsExist(std::vector<FBoneIndexType>& InOutBoneSortedArray) const
{
	
	const int32 NumBones = GetNum();
	// Iterate through existing array.
	uint32 i = 0;

	std::vector<bool>& BoneExists = FEnsureParentsExistScratchArea::Get().BoneExists;
	BoneExists.clear();
	BoneExists.resize(NumBones);

	while (i < InOutBoneSortedArray.size())
	{
		const int32 BoneIndex = InOutBoneSortedArray[i];

		// For the root bone, just move on.
		if (BoneIndex > 0)
		{
			BoneExists[BoneIndex] = true;

			const int32 ParentIndex = GetParentIndex(BoneIndex);

			// If we do not have this parent in the array, we add it in this location, and leave 'i' where it is.
			// This can happen if somebody removes bones in the physics asset, then it will try add back in, and in the process, 
			// parent can be missing
			if (!BoneExists[ParentIndex])
			{
				InOutBoneSortedArray.insert(InOutBoneSortedArray.begin() + i, FBoneIndexType());
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
	
}

void FReferenceSkeleton::EnsureParentsExistAndSort(std::vector<FBoneIndexType>& InOutBoneUnsortedArray) const
{
	
	std::sort(InOutBoneUnsortedArray.begin(), InOutBoneUnsortedArray.end());

	EnsureParentsExist(InOutBoneUnsortedArray);

	std::sort(InOutBoneUnsortedArray.begin(), InOutBoneUnsortedArray.end());
	
}
