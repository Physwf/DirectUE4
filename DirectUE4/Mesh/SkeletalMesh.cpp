#include "SkeletalMesh.h"
#include "log.h"

SkeletalMesh::SkeletalMesh()
	: Skeleton(NULL)
{
	ImportedModel = std::make_unique<SkeletalMeshModel>();

}

void SkeletalMesh::CalculateRequiredBones(SkeletalMeshLODModel& LODModel, const struct FReferenceSkeleton& RefSkeleton, const std::map<FBoneIndexType, FBoneIndexType>* BonesToRemove)
{
	// RequiredBones for base model includes all raw bones.
	int32 RequiredBoneCount = RefSkeleton.GetRawBoneNum();
	LODModel.RequiredBones.clear();
	for (int32 i = 0; i < RequiredBoneCount; i++)
	{
		// Make sure it's not in BonesToRemove
		// @Todo change this to one TArray
		if (!BonesToRemove || BonesToRemove->find(i) == BonesToRemove->end())
		{
			LODModel.RequiredBones.push_back(i);
		}
	}

	//LODModel.RequiredBones.Shrink();
}
