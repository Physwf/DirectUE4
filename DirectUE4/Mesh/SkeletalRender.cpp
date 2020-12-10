#include "SkeletalRenderPublic.h"
#include "MeshComponent.h"
#include "SkeletalMeshRenderData.h"
#include "SkeletalMesh.h"

FSkeletalMeshObject::FSkeletalMeshObject(USkinnedMeshComponent* InMeshComponent, FSkeletalMeshRenderData* InSkelMeshRenderData)
	: MinDesiredLODLevel(0)
	/*, MaxDistanceFactor(0.f)*/
	/*, WorkingMinDesiredLODLevel(0)*/
	/*, WorkingMaxDistanceFactor(0.f)*/
	/*, bHasBeenUpdatedAtLeastOnce(false)*/
	, SkeletalMeshRenderData(InSkelMeshRenderData)
	, SkeletalMeshLODInfo(InMeshComponent->SkeletalMesh->GetLODInfoArray())
	//, SkinCacheEntry(nullptr)
{
	InitLODInfos(InMeshComponent);
}

FSkeletalMeshObject::~FSkeletalMeshObject()
{

}

void FSkeletalMeshObject::InitLODInfos(const USkinnedMeshComponent* InMeshComponent)
{
	LODInfo.clear();
	LODInfo.reserve(SkeletalMeshLODInfo.size());
	for (uint32 Idx = 0; Idx < SkeletalMeshLODInfo.size(); Idx++)
	{
		LODInfo.push_back(FSkelMeshObjectLODInfo());
		FSkelMeshObjectLODInfo& MeshLODInfo = LODInfo.back();
		if (IsValidIndex(InMeshComponent->LODInfo,Idx))
		{
			const FSkelMeshComponentLODInfo &Info = InMeshComponent->LODInfo[Idx];

			MeshLODInfo.HiddenMaterials = Info.HiddenMaterials;
		}
	}
}

