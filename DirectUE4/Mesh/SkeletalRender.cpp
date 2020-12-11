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

const std::vector<FSkeletalMeshRenderSection>& FSkeletalMeshObject::GetRenderSections(int32 InLODIndex) const
{
	const FSkeletalMeshLODRenderData& LOD = *SkeletalMeshRenderData->LODRenderData[InLODIndex];
	return LOD.RenderSections;
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

void UpdateRefToLocalMatricesInner(std::vector<FMatrix>& ReferenceToLocal, const std::vector<FTransform>& ComponentTransform, const std::vector<uint8>& BoneVisibilityStates, const std::vector<int32>* MasterBoneMap,
	const std::vector<FMatrix>* RefBasesInvMatrix, const FReferenceSkeleton& RefSkeleton, const FSkeletalMeshRenderData* InSkeletalMeshRenderData, int32 LODIndex, const std::vector<FBoneIndexType>* ExtraRequiredBoneIndices)
{
	const FSkeletalMeshLODRenderData& LOD = *InSkeletalMeshRenderData->LODRenderData[LODIndex];

	assert(RefBasesInvMatrix->size() != 0);

	if (ReferenceToLocal.size() != RefBasesInvMatrix->size())
	{
		ReferenceToLocal.clear();
		ReferenceToLocal.resize(RefBasesInvMatrix->size());
	}

	const std::vector<FBoneIndexType>* RequiredBoneSets[3] = { &LOD.ActiveBoneIndices, ExtraRequiredBoneIndices, NULL };

	const bool bBoneVisibilityStatesValid = BoneVisibilityStates.size() == ComponentTransform.size();
	const bool bIsMasterCompValid = MasterBoneMap != nullptr;

	for (int32 RequiredBoneSetIndex = 0; RequiredBoneSets[RequiredBoneSetIndex] != NULL; RequiredBoneSetIndex++)
	{
		const std::vector<FBoneIndexType>& RequiredBoneIndices = *RequiredBoneSets[RequiredBoneSetIndex];

		// Get the index of the bone in this skeleton, and loop up in table to find index in parent component mesh.
		for (uint32 BoneIndex = 0; BoneIndex < RequiredBoneIndices.size(); BoneIndex++)
		{
			const int32 ThisBoneIndex = RequiredBoneIndices[BoneIndex];

			if (IsValidIndex(*RefBasesInvMatrix, ThisBoneIndex))
			{
				ReferenceToLocal[ThisBoneIndex] = FMatrix::Identity;
				if (bIsMasterCompValid)
				{
					const int32 MasterBoneIndex = (*MasterBoneMap)[ThisBoneIndex];
					if (IsValidIndex(ComponentTransform,MasterBoneIndex))
					{
						const int32 ParentIndex = RefSkeleton.GetParentIndex(ThisBoneIndex);
						bool bNeedToHideBone = BoneVisibilityStates[MasterBoneIndex] != BVS_Visible;
						if (bNeedToHideBone && ParentIndex != INDEX_NONE)
						{
							ReferenceToLocal[ThisBoneIndex] = ReferenceToLocal[ParentIndex].ApplyScale(0.f);
						}
						else
						{
							assert(ComponentTransform[MasterBoneIndex].IsRotationNormalized());
							ReferenceToLocal[ThisBoneIndex] = ComponentTransform[MasterBoneIndex].ToMatrixWithScale();
						}
					}
					else
					{
						const int32 ParentIndex = RefSkeleton.GetParentIndex(ThisBoneIndex);
						const FMatrix RefLocalPose = RefSkeleton.GetRefBonePose()[ThisBoneIndex].ToMatrixWithScale();
						if (ParentIndex != INDEX_NONE)
						{
							ReferenceToLocal[ThisBoneIndex] = RefLocalPose * ReferenceToLocal[ParentIndex];
						}
						else
						{
							ReferenceToLocal[ThisBoneIndex] = RefLocalPose;
						}
					}
				}
				else
				{
					if (IsValidIndex(ComponentTransform,ThisBoneIndex))
					{
						if (bBoneVisibilityStatesValid)
						{
							// If we can't find this bone in the parent, we just use the reference pose.
							const int32 ParentIndex = RefSkeleton.GetParentIndex(ThisBoneIndex);
							bool bNeedToHideBone = BoneVisibilityStates[ThisBoneIndex] != BVS_Visible;
							if (bNeedToHideBone && ParentIndex != INDEX_NONE)
							{
								ReferenceToLocal[ThisBoneIndex] = ReferenceToLocal[ParentIndex].ApplyScale(0.f);
							}
							else
							{
								assert(ComponentTransform[ThisBoneIndex].IsRotationNormalized());
								ReferenceToLocal[ThisBoneIndex] = ComponentTransform[ThisBoneIndex].ToMatrixWithScale();
							}
						}
						else
						{
							assert(ComponentTransform[ThisBoneIndex].IsRotationNormalized());
							ReferenceToLocal[ThisBoneIndex] = ComponentTransform[ThisBoneIndex].ToMatrixWithScale();
						}
					}
				}
			}
		}
	}

	for (uint32 ThisBoneIndex = 0; ThisBoneIndex < ReferenceToLocal.size(); ++ThisBoneIndex)
	{
		ReferenceToLocal[ThisBoneIndex] = (*RefBasesInvMatrix)[ThisBoneIndex] * ReferenceToLocal[ThisBoneIndex];
	}
}

void UpdateRefToLocalMatrices(std::vector<FMatrix>& ReferenceToLocal, const USkinnedMeshComponent* InMeshComponent, const FSkeletalMeshRenderData* InSkeletalMeshRenderData, int32 LODIndex, const std::vector<FBoneIndexType>* ExtraRequiredBoneIndices /*= NULL*/)
{
	const USkeletalMesh* const ThisMesh = InMeshComponent->SkeletalMesh;
	const USkinnedMeshComponent* const MasterComp = InMeshComponent->MasterPoseComponent.lock().get();
	const FSkeletalMeshLODRenderData& LOD = *InSkeletalMeshRenderData->LODRenderData[LODIndex];

	const FReferenceSkeleton& RefSkeleton = ThisMesh->RefSkeleton;
	const std::vector<int32>& MasterBoneMap = InMeshComponent->GetMasterBoneMap();
	const bool bIsMasterCompValid = MasterComp && MasterBoneMap.size() == ThisMesh->RefSkeleton.GetNum();
	const std::vector<FTransform>& ComponentTransform = (bIsMasterCompValid) ? MasterComp->GetComponentSpaceTransforms() : InMeshComponent->GetComponentSpaceTransforms();
	const std::vector<uint8>& BoneVisibilityStates = (bIsMasterCompValid) ? MasterComp->BoneVisibilityStates : InMeshComponent->BoneVisibilityStates;
	// Get inv ref pose matrices
	const std::vector<FMatrix>* RefBasesInvMatrix = &ThisMesh->RefBasesInvMatrix;

	if (ReferenceToLocal.size() != RefBasesInvMatrix->size())
	{
		ReferenceToLocal.clear();
		ReferenceToLocal.resize(RefBasesInvMatrix->size());
	}

	UpdateRefToLocalMatricesInner(ReferenceToLocal, ComponentTransform, BoneVisibilityStates, (bIsMasterCompValid) ? &MasterBoneMap : nullptr, RefBasesInvMatrix, RefSkeleton, InSkeletalMeshRenderData, LODIndex, ExtraRequiredBoneIndices);
}

void UpdatePreviousRefToLocalMatrices(std::vector<FMatrix>& ReferenceToLocal, const USkinnedMeshComponent* InMeshComponent, const FSkeletalMeshRenderData* InSkeletalMeshRenderData, int32 LODIndex, const std::vector<FBoneIndexType>* ExtraRequiredBoneIndices /*= NULL*/)
{
	const USkeletalMesh* const ThisMesh = InMeshComponent->SkeletalMesh;
	const USkinnedMeshComponent* const MasterComp = InMeshComponent->MasterPoseComponent.lock().get();
	const FSkeletalMeshLODRenderData& LOD = *InSkeletalMeshRenderData->LODRenderData[LODIndex];

	const FReferenceSkeleton& RefSkeleton = ThisMesh->RefSkeleton;
	const std::vector<int32>& MasterBoneMap = InMeshComponent->GetMasterBoneMap();
	const bool bIsMasterCompValid = MasterComp && MasterBoneMap.size() == ThisMesh->RefSkeleton.GetNum();
	const std::vector<FTransform>& ComponentTransform = (bIsMasterCompValid) ? MasterComp->GetPreviousComponentTransformsArray() : InMeshComponent->GetPreviousComponentTransformsArray();
	const std::vector<uint8>& BoneVisibilityStates = (bIsMasterCompValid) ? MasterComp->GetPreviousBoneVisibilityStates() : InMeshComponent->GetPreviousBoneVisibilityStates();
	// Get inv ref pose matrices
	const std::vector<FMatrix>* RefBasesInvMatrix = &ThisMesh->RefBasesInvMatrix;

	if (ReferenceToLocal.size() != RefBasesInvMatrix->size())
	{
		ReferenceToLocal.clear();
		ReferenceToLocal.resize(RefBasesInvMatrix->size());
	}

	UpdateRefToLocalMatricesInner(ReferenceToLocal, ComponentTransform, BoneVisibilityStates, (bIsMasterCompValid) ? &MasterBoneMap : nullptr, RefBasesInvMatrix, RefSkeleton, InSkeletalMeshRenderData, LODIndex, ExtraRequiredBoneIndices);
}

