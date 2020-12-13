#include "MeshComponent.h"
#include "StaticMeshResources.h"
#include "StaticMesh.h"
#include "World.h"
#include "MapBuildDataRegistry.h"
#include "SkeletalMeshTypes.h"
#include "SkeletalRenderGPUSkin.h"
#include "SkeletalMesh.h"

UMeshComponent::UMeshComponent(AActor* InOwner)
	: UPrimitiveComponent(InOwner)
{

}

UMeshComponent::~UMeshComponent()
{

}

bool UStaticMeshComponent::SetStaticMesh(class UStaticMesh* NewMesh)
{
	StaticMesh = NewMesh;
	UpdateBounds();
	return true;
}

UMaterial* UStaticMeshComponent::GetMaterial(int32 MaterialIndex) const
{
	return GetStaticMesh() ? GetStaticMesh()->GetMaterial(MaterialIndex) : nullptr;
}

FPrimitiveSceneProxy* UStaticMeshComponent::CreateSceneProxy()
{
	return new FStaticMeshSceneProxy(this,false);
}

const class FMeshMapBuildData* UStaticMeshComponent::GetMeshMapBuildData() const
{
	UMapBuildDataRegistry* MapBuildData = GetOwner()->GetWorld()->MapBuildData;
	return MapBuildData->GetMeshBuildData(0);
}

FSkelMeshComponentLODInfo::FSkelMeshComponentLODInfo()
{

}

FSkelMeshComponentLODInfo::~FSkelMeshComponentLODInfo()
{

}


FSkeletalMeshRenderData* USkinnedMeshComponent::GetSkeletalMeshRenderData() const
{
	if (MeshObject)
	{
		return &MeshObject->GetSkeletalMeshRenderData();
	}
	else if (SkeletalMesh)
	{
		return SkeletalMesh->GetResourceForRendering();
	}
	return NULL;
}

void USkinnedMeshComponent::SetSkeletalMesh(class USkeletalMesh* InSkelMesh, bool bReinitPose /*= true*/)
{
	SkeletalMesh = InSkelMesh;

	if (IsRegistered())
	{
		AllocateTransformData();
		UpdateMasterBoneMap();
		//InvalidateCachedBounds();
		// clear morphtarget cache
		//ActiveMorphTargets.Empty();
		//MorphTargetWeights.Empty();
	}
}

void USkinnedMeshComponent::SetMasterPoseComponent(USkinnedMeshComponent* NewMasterBoneComponent, bool bForceUpdate /*= false*/)
{

}

void USkinnedMeshComponent::OnRegister()
{
	if (!MasterPoseComponent.expired())
	{
		// we have to make sure it updates the mastesr pose
		SetMasterPoseComponent(MasterPoseComponent.lock().get(), true);
	}
	else
	{
		AllocateTransformData();
	}

	UMeshComponent::OnRegister();

	UpdateLODStatus();
}

void USkinnedMeshComponent::OnUnregister()
{
	DeallocateTransformData();
	UMeshComponent::OnUnregister();
}

void USkinnedMeshComponent::CreateRenderState_Concurrent()
{
	InitLODInfos();

	FSkeletalMeshRenderData* SkelMeshRenderData = SkeletalMesh->GetResourceForRendering();

	const bool bIsCPUSkinned = false;// SkelMeshRenderData->RequiresCPUSkinning(SceneFeatureLevel) || ShouldCPUSkin();

	MeshObject = ::new FSkeletalMeshObjectGPUSkin(this, SkelMeshRenderData);

	UMeshComponent::CreateRenderState_Concurrent();

	if (SkeletalMesh)
	{
		// Update dynamic data

		if (MeshObject)
		{
			// Calculate new lod level
			UpdateLODStatus();

			// If we have a valid LOD, set up required data, during reimport we may try to create data before we have all the LODs
			// imported, in that case we skip until we have all the LODs
			if (SkeletalMesh->IsValidLODIndex(PredictedLODLevel))
			{
// 				const bool bMorphTargetsAllowed = CVarEnableMorphTargets.GetValueOnAnyThread(true) != 0;
// 
// 				// Are morph targets disabled for this LOD?
// 				if (bDisableMorphTarget || !bMorphTargetsAllowed)
// 				{
// 					ActiveMorphTargets.Empty();
// 				}

				MeshObject->Update(PredictedLODLevel, this, /*ActiveMorphTargets, MorphTargetWeights,*/ true);  // send to rendering thread
			}
		}

		// scene proxy update of material usage based on active morphs
		//UpdateMorphMaterialUsageOnProxy();
	}
}

void USkinnedMeshComponent::SendRenderDynamicData_Concurrent()
{
	UMeshComponent::SendRenderDynamicData_Concurrent();

	if (MeshObject && SkeletalMesh)
	{

		const int32 UseLOD = 0;// PredictedLODLevel;

		//const bool bMorphTargetsAllowed = CVarEnableMorphTargets.GetValueOnAnyThread(true) != 0;

		// Are morph targets disabled for this LOD?
// 		if (bDisableMorphTarget || !bMorphTargetsAllowed)
// 		{
// 			ActiveMorphTargets.Empty();
// 		}

		//check(UseLOD < MeshObject->GetSkeletalMeshRenderData().LODRenderData.Num());
		MeshObject->Update(UseLOD, this, /*ActiveMorphTargets, MorphTargetWeights,*/ false);  // send to rendering thread
		//MeshObject->bHasBeenUpdatedAtLeastOnce = true;

		// scene proxy update of material usage based on active morphs
		//UpdateMorphMaterialUsageOnProxy();
	}
}

void USkinnedMeshComponent::DestroyRenderState_Concurrent()
{
	UMeshComponent::DestroyRenderState_Concurrent();

	// clear morphtarget array info while rendering state is destroyed
	//ActiveMorphTargets.Empty();
	//MorphTargetWeights.Empty();

	if (MeshObject)
	{
		// Begin releasing the RHI resources used by this skeletal mesh component.
		// This doesn't immediately destroy anything, since the rendering thread may still be using the resources.
		MeshObject->ReleaseResources();

		// Begin a deferred delete of MeshObject.  BeginCleanup will call MeshObject->FinishDestroy after the above release resource
		// commands execute in the rendering thread.
		//BeginCleanup(MeshObject);
		MeshObject = NULL;
	}
}

bool USkinnedMeshComponent::UpdateLODStatus()
{
	PredictedLODLevel = 0;
	return true;
}

void USkinnedMeshComponent::UpdateMasterBoneMap()
{
	MasterBoneMap.clear();
}

FPrimitiveSceneProxy* USkinnedMeshComponent::CreateSceneProxy()
{
	FSkeletalMeshSceneProxy* Result = nullptr;
	FSkeletalMeshRenderData* SkelMeshRenderData = GetSkeletalMeshRenderData();

	if (SkelMeshRenderData &&
// 		SkelMeshRenderData->LODRenderData.IsValidIndex(PredictedLODLevel) &&
// 		!bHideSkin &&
		MeshObject)
	{
		// Only create a scene proxy if the bone count being used is supported, or if we don't have a skeleton (this is the case with destructibles)
// 		int32 MaxBonesPerChunk = SkelMeshRenderData->GetMaxBonesPerSection();
// 		if (MaxBonesPerChunk <= GetFeatureLevelMaxNumberOfBones(SceneFeatureLevel))
		{
			Result = ::new FSkeletalMeshSceneProxy(this, SkelMeshRenderData);
		}
	}

	return Result;
}

class UMaterial* USkinnedMeshComponent::GetMaterial(int32 MaterialIndex) const
{
// 	if (OverrideMaterials.IsValidIndex(MaterialIndex) && OverrideMaterials[MaterialIndex])
// 	{
// 		return OverrideMaterials[MaterialIndex];
// 	}
	/*else*/ if (SkeletalMesh && IsValidIndex(SkeletalMesh->Materials, MaterialIndex) && SkeletalMesh->Materials[MaterialIndex].MaterialInterface)
	{
		return SkeletalMesh->Materials[MaterialIndex].MaterialInterface;
	}
	
	return nullptr;
}

void USkinnedMeshComponent::InitLODInfos()
{
	if (SkeletalMesh != NULL)
	{
		if (SkeletalMesh->GetLODNum() != LODInfo.size())
		{
			LODInfo.clear();
			LODInfo.reserve(SkeletalMesh->GetLODNum());
			for (uint32 Idx = 0; Idx < SkeletalMesh->GetLODNum(); Idx++)
			{
				LODInfo.push_back(FSkelMeshComponentLODInfo());
			}
		}
	}
}

void USkinnedMeshComponent::FlipEditableSpaceBases()
{
	if (bNeedToFlipSpaceBaseBuffers)
	{
		// save previous transform if it's valid
		if (bHasValidBoneTransform)
		{
			PreviousComponentSpaceTransformsArray = GetComponentSpaceTransforms();
			PreviousBoneVisibilityStates = BoneVisibilityStates;
		}

		bNeedToFlipSpaceBaseBuffers = false;
		if (bDoubleBufferedComponentSpaceTransforms)
		{
			CurrentReadComponentTransforms = CurrentEditableComponentTransforms;
			CurrentEditableComponentTransforms = 1 - CurrentEditableComponentTransforms;
		}
		else
		{
			CurrentReadComponentTransforms = CurrentEditableComponentTransforms = 0;
		}

		// if we don't have a valid transform, we copy after we write, so that it doesn't cause motion blur
		if (!bHasValidBoneTransform)
		{
			PreviousComponentSpaceTransformsArray = GetComponentSpaceTransforms();
			PreviousBoneVisibilityStates = BoneVisibilityStates;
		}

		++CurrentBoneTransformRevisionNumber;
	}
}

bool USkinnedMeshComponent::AllocateTransformData()
{
	if (SkeletalMesh != NULL && MasterPoseComponent.lock() == NULL)
	{
		if (GetNumComponentSpaceTransforms() != SkeletalMesh->RefSkeleton.GetNum())
		{
			for (int32 BaseIndex = 0; BaseIndex < 2; ++BaseIndex)
			{
				ComponentSpaceTransformsArray[BaseIndex].clear();
				ComponentSpaceTransformsArray[BaseIndex].reserve(SkeletalMesh->RefSkeleton.GetNum());
				ComponentSpaceTransformsArray[BaseIndex].resize(SkeletalMesh->RefSkeleton.GetNum());

				for (int32 I = 0; I < SkeletalMesh->RefSkeleton.GetNum(); ++I)
				{
					ComponentSpaceTransformsArray[BaseIndex][I].SetIdentity();
				}
			}
			BoneVisibilityStates.clear();
			BoneVisibilityStates.reserve(SkeletalMesh->RefSkeleton.GetNum());
			if (SkeletalMesh->RefSkeleton.GetNum())
			{
				BoneVisibilityStates.resize(SkeletalMesh->RefSkeleton.GetNum());
				for (int32 BoneIndex = 0; BoneIndex < SkeletalMesh->RefSkeleton.GetNum(); BoneIndex++)
				{
					BoneVisibilityStates[BoneIndex] = BVS_Visible;
				}
			}

			// when initialize bone transform first time
			// it is invalid
			bHasValidBoneTransform = false;
			PreviousComponentSpaceTransformsArray = ComponentSpaceTransformsArray[0];
			PreviousBoneVisibilityStates = BoneVisibilityStates;
		}

		// if it's same, do not touch, and return
		return true;
	}

	// Reset the animation stuff when changing mesh.
	ComponentSpaceTransformsArray[0].clear();
	ComponentSpaceTransformsArray[1].clear();
	PreviousComponentSpaceTransformsArray.clear();
	return false;
}

void USkinnedMeshComponent::DeallocateTransformData()
{

}

void USkeletalMeshComponent::SetSkeletalMesh(class USkeletalMesh* InSkelMesh, bool bReinitPose /*= true*/)
{
	USkinnedMeshComponent::SetSkeletalMesh(InSkelMesh, bReinitPose);

	InitAnim(bReinitPose);
}

void USkeletalMeshComponent::InitAnim(bool bForceReinit)
{
	if (SkeletalMesh != nullptr && IsRegistered())
	{
		RecalcRequiredBones(PredictedLODLevel);

		BoneSpaceTransforms = SkeletalMesh->RefSkeleton.GetRefBonePose();
		//Mini RefreshBoneTransforms (the bit we actually care about)
		FillComponentSpaceTransforms(SkeletalMesh, BoneSpaceTransforms, GetEditableComponentSpaceTransforms());
		bNeedToFlipSpaceBaseBuffers = true; // Have updated space bases so need to flip
		FlipEditableSpaceBases();
	}
}

void USkeletalMeshComponent::RecalcRequiredBones(int32 LODIndex)
{
	if (!SkeletalMesh)
	{
		return;
	}

	ComputeRequiredBones(RequiredBones, FillComponentSpaceTransformsRequiredBones, LODIndex, /*bIgnorePhysicsAsset=*/ false);

	BoneSpaceTransforms = SkeletalMesh->RefSkeleton.GetRefBonePose();

}

void USkeletalMeshComponent::ComputeRequiredBones(std::vector<FBoneIndexType>& OutRequiredBones, std::vector<FBoneIndexType>& OutFillComponentSpaceTransformsRequiredBones, int32 LODIndex, bool bIgnorePhysicsAsset) const
{
	OutRequiredBones.clear();
	OutFillComponentSpaceTransformsRequiredBones.clear();

	if (!SkeletalMesh)
	{
		return;
	}

	FSkeletalMeshRenderData* SkelMeshRenderData = GetSkeletalMeshRenderData();
	assert(SkelMeshRenderData);

	LODIndex = FMath::Clamp(LODIndex, 0, (int32)SkelMeshRenderData->LODRenderData.size() - 1);

	// The list of bones we want is taken from the predicted LOD level.
	FSkeletalMeshLODRenderData& LODData = *SkelMeshRenderData->LODRenderData[LODIndex];
	OutRequiredBones = LODData.RequiredBones;

	OutFillComponentSpaceTransformsRequiredBones = OutRequiredBones;
}

void USkeletalMeshComponent::OnRegister()
{
	USkinnedMeshComponent::OnRegister();
	InitAnim(true);
}

void USkeletalMeshComponent::OnUnregister()
{
	USkinnedMeshComponent::OnUnregister();
}

void USkeletalMeshComponent::FillComponentSpaceTransforms(const USkeletalMesh* InSkeletalMesh, const std::vector<FTransform>& InBoneSpaceTransforms, std::vector<FTransform>& OutComponentSpaceTransforms) const
{
	if (!InSkeletalMesh)
	{
		return;
	}

	assert(InSkeletalMesh->RefSkeleton.GetNum() == InBoneSpaceTransforms.size());
	assert(InSkeletalMesh->RefSkeleton.GetNum() == OutComponentSpaceTransforms.size());

	const FTransform* LocalTransformsData = InBoneSpaceTransforms.data();
	FTransform* ComponentSpaceData = OutComponentSpaceTransforms.data();

	{
		assert(FillComponentSpaceTransformsRequiredBones[0] == 0);
		OutComponentSpaceTransforms[0] = InBoneSpaceTransforms[0];
	}

	for (uint32 i = 1; i < FillComponentSpaceTransformsRequiredBones.size(); i++)
	{
		const int32 BoneIndex = FillComponentSpaceTransformsRequiredBones[i];
		FTransform* SpaceBase = ComponentSpaceData + BoneIndex;


		const int32 ParentIndex = InSkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
		FTransform* ParentSpaceBase = ComponentSpaceData + ParentIndex;

		FTransform::Multiply(SpaceBase, LocalTransformsData + BoneIndex, ParentSpaceBase);

		SpaceBase->NormalizeRotation();
	}

}
