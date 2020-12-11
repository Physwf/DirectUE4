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

	//UpdateLODStatus();
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

class UMaterial* USkinnedMeshComponent::GetMaterial(int32 ElementIndex) const
{
	/*
	if (OverrideMaterials.IsValidIndex(MaterialIndex) && OverrideMaterials[MaterialIndex])
	{
		return OverrideMaterials[MaterialIndex];
	}
	else if (SkeletalMesh && IsValidIndex(SkeletalMesh->Materials, MaterialIndex) && SkeletalMesh->Materials[MaterialIndex].MaterialInterface)
	{
		return SkeletalMesh->Materials[MaterialIndex].MaterialInterface;
	}
	*/
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

