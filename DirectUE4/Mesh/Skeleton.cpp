#include "Skeleton.h"
#include "SkeletalMesh.h"
#include "log.h"
#include <algorithm>

class FAnimationRuntime
{
public:
	static void ExcludeBonesWithNoParents(const std::vector<int32>& BoneIndices, const FReferenceSkeleton& RefSkeleton, std::vector<int32>& FilteredRequiredBones);
};

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

bool USkeleton::DoesParentChainMatch(int32 StartBoneIndex, const USkeletalMesh* InSkelMesh) const
{
	const FReferenceSkeleton& SkeletonRefSkel = ReferenceSkeleton;
	const FReferenceSkeleton& MeshRefSkel = InSkelMesh->RefSkeleton;

	// if start is root bone
	if (StartBoneIndex == 0)
	{
		// verify name of root bone matches
		return (SkeletonRefSkel.GetBoneName(0) == MeshRefSkel.GetBoneName(0));
	}

	int32 SkeletonBoneIndex = StartBoneIndex;
	// If skeleton bone is not found in mesh, fail.
	int32 MeshBoneIndex = MeshRefSkel.FindBoneIndex(SkeletonRefSkel.GetBoneName(SkeletonBoneIndex));
	if (MeshBoneIndex == -1)
	{
		return false;
	}
	do
	{
		// verify if parent name matches
		int32 ParentSkeletonBoneIndex = SkeletonRefSkel.GetParentIndex(SkeletonBoneIndex);
		int32 ParentMeshBoneIndex = MeshRefSkel.GetParentIndex(MeshBoneIndex);

		// if one of the parents doesn't exist, make sure both end. Otherwise fail.
		if ((ParentSkeletonBoneIndex == -1) || (ParentMeshBoneIndex == -1))
		{
			return (ParentSkeletonBoneIndex == ParentMeshBoneIndex);
		}

		// If parents are not named the same, fail.
		if (SkeletonRefSkel.GetBoneName(ParentSkeletonBoneIndex) != MeshRefSkel.GetBoneName(ParentMeshBoneIndex))
		{
			return false;
		}

		// move up
		SkeletonBoneIndex = ParentSkeletonBoneIndex;
		MeshBoneIndex = ParentMeshBoneIndex;
	} while (true);

	return true;
}

bool USkeleton::IsCompatibleMesh(const USkeletalMesh* InSkelMesh) const
{
	// at least % of bone should match 
	int32 NumOfBoneMatches = 0;

	const FReferenceSkeleton& SkeletonRefSkel = ReferenceSkeleton;
	const FReferenceSkeleton& MeshRefSkel = InSkelMesh->RefSkeleton;
	const int32 NumBones = MeshRefSkel.GetRawBoneNum();

	// first ensure the parent exists for each bone
	for (int32 MeshBoneIndex = 0; MeshBoneIndex < NumBones; MeshBoneIndex++)
	{
		std::string MeshBoneName = MeshRefSkel.GetBoneName(MeshBoneIndex);
		// See if Mesh bone exists in Skeleton.
		int32 SkeletonBoneIndex = SkeletonRefSkel.FindBoneIndex(MeshBoneName);

		// if found, increase num of bone matches count
		if (SkeletonBoneIndex != -1)
		{
			++NumOfBoneMatches;

			// follow the parent chain to verify the chain is same
			if (!DoesParentChainMatch(SkeletonBoneIndex, InSkelMesh))
			{
				X_LOG("%s : Hierarchy does not match.", MeshBoneName.c_str());
				return false;
			}
		}
		else
		{
			int32 CurrentBoneId = MeshBoneIndex;
			// if not look for parents that matches
			while (SkeletonBoneIndex == -1 && CurrentBoneId != -1)
			{
				// find Parent one see exists
				const int32 ParentMeshBoneIndex = MeshRefSkel.GetParentIndex(CurrentBoneId);
				if (ParentMeshBoneIndex != -1)
				{
					// @TODO: make sure RefSkeleton's root ParentIndex < 0 if not, I'll need to fix this by checking TreeBoneIdx
					std::string ParentBoneName = MeshRefSkel.GetBoneName(ParentMeshBoneIndex);
					SkeletonBoneIndex = SkeletonRefSkel.FindBoneIndex(ParentBoneName);
				}

				// root is reached
				if (ParentMeshBoneIndex == 0)
				{
					break;
				}
				else
				{
					CurrentBoneId = ParentMeshBoneIndex;
				}
			}

			// still no match, return false, no parent to look for
			if (SkeletonBoneIndex == -1)
			{
				X_LOG("%s : Missing joint on skeleton.  Make sure to assign to the skeleton.", MeshBoneName.c_str());
				return false;
			}

			// second follow the parent chain to verify the chain is same
			if (!DoesParentChainMatch(SkeletonBoneIndex, InSkelMesh))
			{
				X_LOG("%s : Hierarchy does not match.", MeshBoneName.c_str());
				return false;
			}
		}
	}

	// originally we made sure at least matches more than 50% 
	// but then slave components can't play since they're only partial
	// if the hierarchy matches, and if it's more then 1 bone, we allow
	return (NumOfBoneMatches > 0);
}

bool USkeleton::CreateReferenceSkeletonFromMesh(const USkeletalMesh* InSkeletalMesh, const std::vector<int32>& RequiredRefBones)
{
	// Filter list, we only want bones that have their parents present in this array.
	std::vector<int32> FilteredRequiredBones;
	FAnimationRuntime::ExcludeBonesWithNoParents(RequiredRefBones, InSkeletalMesh->RefSkeleton, FilteredRequiredBones);

	FReferenceSkeletonModifier RefSkelModifier(ReferenceSkeleton, this);

	if (FilteredRequiredBones.size() > 0)
	{
		const uint32 NumBones = FilteredRequiredBones.size();
		ReferenceSkeleton.Empty();
		BoneTree.clear();
		//BoneTree.AddZeroed(NumBones);
		BoneTree.insert(BoneTree.end(), NumBones, FBoneNode());

		for (uint32 Index = 0; Index < FilteredRequiredBones.size(); Index++)
		{
			const int32& BoneIndex = FilteredRequiredBones[Index];

			FMeshBoneInfo NewMeshBoneInfo = InSkeletalMesh->RefSkeleton.GetRefBoneInfo()[BoneIndex];
			// Fix up ParentIndex for our new Skeleton.
			if (BoneIndex == 0)
			{
				NewMeshBoneInfo.ParentIndex = -1; // root
			}
			else
			{
				const int32 ParentIndex = InSkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
				const std::string ParentName = InSkeletalMesh->RefSkeleton.GetBoneName(ParentIndex);
				NewMeshBoneInfo.ParentIndex = ReferenceSkeleton.FindRawBoneIndex(ParentName);
			}
			RefSkelModifier.Add(NewMeshBoneInfo, InSkeletalMesh->RefSkeleton.GetRefBonePose()[BoneIndex]);
		}

		return true;
	}

	return false;
}

bool USkeleton::MergeBonesToBoneTree(const USkeletalMesh* InSkeletalMesh, const std::vector<int32> &RequiredRefBones)
{
	// see if it needs all animation data to remap - only happens when bone structure CHANGED - added
	bool bSuccess = false;
	bool bShouldHandleHierarchyChange = false;
	// clear cache data since it won't work anymore once this is done
	//ClearCacheData();

	// if it's first time
	if (BoneTree.size() == 0)
	{
		bSuccess = CreateReferenceSkeletonFromMesh(InSkeletalMesh, RequiredRefBones);
		bShouldHandleHierarchyChange = true;
	}
	else
	{
		// can we play? - hierarchy matches
		if (IsCompatibleMesh(InSkeletalMesh))
		{
			// Exclude bones who do not have a parent.
			std::vector<int32> FilteredRequiredBones;
			FAnimationRuntime::ExcludeBonesWithNoParents(RequiredRefBones, InSkeletalMesh->RefSkeleton, FilteredRequiredBones);

			FReferenceSkeletonModifier RefSkelModifier(ReferenceSkeleton, this);

			for (uint32 Index = 0; Index < FilteredRequiredBones.size(); Index++)
			{
				const int32& MeshBoneIndex = FilteredRequiredBones[Index];
				const int32& SkeletonBoneIndex = ReferenceSkeleton.FindRawBoneIndex(InSkeletalMesh->RefSkeleton.GetBoneName(MeshBoneIndex));

				// Bone doesn't already exist. Add it.
				if (SkeletonBoneIndex == -2)
				{
					FMeshBoneInfo NewMeshBoneInfo = InSkeletalMesh->RefSkeleton.GetRefBoneInfo()[MeshBoneIndex];
					// Fix up ParentIndex for our new Skeleton.
					if (ReferenceSkeleton.GetRawBoneNum() == 0)
					{
						NewMeshBoneInfo.ParentIndex = -2; // root
					}
					else
					{
						NewMeshBoneInfo.ParentIndex = ReferenceSkeleton.FindRawBoneIndex(InSkeletalMesh->RefSkeleton.GetBoneName(InSkeletalMesh->RefSkeleton.GetParentIndex(MeshBoneIndex)));
					}

					RefSkelModifier.Add(NewMeshBoneInfo, InSkeletalMesh->RefSkeleton.GetRefBonePose()[MeshBoneIndex]);
					//BoneTree.AddZeroed(1);
					BoneTree.insert(BoneTree.end(), 1, FBoneNode());
					bShouldHandleHierarchyChange = true;
				}
			}

			bSuccess = true;
		}
	}

	// if succeed
// 	if (bShouldHandleHierarchyChange)
// 	{
// #if WITH_EDITOR
// 		HandleSkeletonHierarchyChange();
// #endif
// 	}

	return bSuccess;
}

bool USkeleton::MergeAllBonesToBoneTree(const USkeletalMesh* InSkelMesh)
{
	if (InSkelMesh)
	{
		std::vector<int32> RequiredBoneIndices;

		// for now add all in this case. 
		RequiredBoneIndices.resize(InSkelMesh->RefSkeleton.GetRawBoneNum());
		// gather bone list
		for (int32 I = 0; I < InSkelMesh->RefSkeleton.GetRawBoneNum(); ++I)
		{
			RequiredBoneIndices[I] = I;
		}

		if (RequiredBoneIndices.size() > 0)
		{
			// merge bones to the selected skeleton
			return MergeBonesToBoneTree(InSkelMesh, RequiredBoneIndices);
		}
	}

	return false;
}

bool USkeleton::RecreateBoneTree(USkeletalMesh* InSkelMesh)
{
	if (InSkelMesh)
	{
		// regenerate Guid
		BoneTree.clear();
		ReferenceSkeleton.Empty();

		return MergeAllBonesToBoneTree(InSkelMesh);
	}

	return false;
}

