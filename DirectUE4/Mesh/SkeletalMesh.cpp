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

void SkeletalMesh::PostLoad()
{
	CacheDerivedData();
	InitResources();
}

void SkeletalMesh::InitResources()
{
	//UpdateUVChannelData(false);

	SkeletalMeshRenderData* SkelMeshRenderData = GetResourceForRendering();

	if (SkelMeshRenderData)
	{
		SkelMeshRenderData->InitResources(/*bHasVertexColors, MorphTargets*/);
	}
}

void SkeletalMesh::ReleaseResources()
{

}

void SkeletalMesh::AllocateResourceForRendering()
{
	RenderdData = std::make_unique<SkeletalMeshRenderData>();
}


void SkeletalMesh::CacheDerivedData()
{
	AllocateResourceForRendering();
	RenderdData->Cache(this);
	//PostMeshCached.Broadcast(this);
}
