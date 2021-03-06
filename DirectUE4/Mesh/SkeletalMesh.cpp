#include "SkeletalMesh.h"
#include "log.h"
#include "SkeletalMeshTypes.h"
#include "MeshComponent.h"
#include "SkeletalRenderGPUSkin.h"
#include "UnrealTemplates.h"
#include "log.h"

USkeletalMesh::USkeletalMesh(class AActor* InOwner)
	:Skeleton(NULL)
{
	ImportedModel = std::make_unique<SkeletalMeshModel>();
}

void USkeletalMesh::CalculateRequiredBones(FSkeletalMeshLODModel& LODModel, const struct FReferenceSkeleton& RefSkeleton, const std::map<FBoneIndexType, FBoneIndexType>* BonesToRemove)
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

void USkeletalMesh::PostLoad()
{
	CacheDerivedData();
	InitResources();

	CalculateInvRefMatrices();
}

void USkeletalMesh::InitResources()
{
	//UpdateUVChannelData(false);

	FSkeletalMeshRenderData* SkelMeshRenderData = GetResourceForRendering();

	if (SkelMeshRenderData)
	{
		SkelMeshRenderData->InitResources(/*bHasVertexColors, MorphTargets*/);
	}
}

void USkeletalMesh::ReleaseResources()
{

}

void USkeletalMesh::AllocateResourceForRendering()
{
	RenderdData = std::make_unique<FSkeletalMeshRenderData>();
}


void USkeletalMesh::CalculateInvRefMatrices()
{
	const int32 NumRealBones = RefSkeleton.GetRawBoneNum();

	if (RefBasesInvMatrix.size() != NumRealBones)
	{
		RefBasesInvMatrix.clear();
		RefBasesInvMatrix.reserve(NumRealBones);
		RefBasesInvMatrix.resize(NumRealBones);

		// Reset cached mesh-space ref pose
		CachedComposedRefPoseMatrices.clear();
		CachedComposedRefPoseMatrices.reserve(NumRealBones);
		CachedComposedRefPoseMatrices.resize(NumRealBones);

		// Precompute the Mesh.RefBasesInverse.
		for (int32 b = 0; b < NumRealBones; b++)
		{
			// Render the default pose.
			CachedComposedRefPoseMatrices[b] = GetRefPoseMatrix(b);

			// Construct mesh-space skeletal hierarchy.
			if (b > 0)
			{
				int32 Parent = RefSkeleton.GetRawParentIndex(b);
				CachedComposedRefPoseMatrices[b] = CachedComposedRefPoseMatrices[b] * CachedComposedRefPoseMatrices[Parent];
			}

			FVector XAxis, YAxis, ZAxis;

			CachedComposedRefPoseMatrices[b].GetScaledAxes(XAxis, YAxis, ZAxis);
			if (XAxis.IsNearlyZero(SMALL_NUMBER) &&
				YAxis.IsNearlyZero(SMALL_NUMBER) &&
				ZAxis.IsNearlyZero(SMALL_NUMBER))
			{
				// this is not allowed, warn them 
				X_LOG("Reference Pose for joint (%s) includes NIL matrix. Zero scale isn't allowed on ref pose. ", *RefSkeleton.GetBoneName(b).c_str());
			}

			// Precompute inverse so we can use from-refpose-skin vertices.
			RefBasesInvMatrix[b] = CachedComposedRefPoseMatrices[b].Inverse();
		}
	}
}

FMatrix USkeletalMesh::GetRefPoseMatrix(int32 BoneIndex) const
{
	assert(BoneIndex >= 0 && BoneIndex < RefSkeleton.GetRawBoneNum());
	FTransform BoneTransform = RefSkeleton.GetRawRefBonePose()[BoneIndex];
	// Make sure quaternion is normalized!
	BoneTransform.NormalizeRotation();
	return BoneTransform.ToMatrixWithScale();
}

FSkeletalMeshLODInfo& USkeletalMesh::AddLODInfo()
{
	uint32 NewIndex = LODInfo.size();
	LODInfo.push_back(FSkeletalMeshLODInfo());
	return LODInfo[NewIndex];
}

void USkeletalMesh::RemoveLODInfo(int32 Index)
{
	if (IsValidIndex(LODInfo,Index))
	{
		LODInfo.erase(LODInfo.begin() + Index);
	}
}

void USkeletalMesh::ResetLODInfo()
{
	LODInfo.clear();
}

void USkeletalMesh::CacheDerivedData()
{
	AllocateResourceForRendering();
	RenderdData->Cache(this);
	//PostMeshCached.Broadcast(this);
}

FSkeletalMeshSceneProxy::FSkeletalMeshSceneProxy(const USkinnedMeshComponent* Component, FSkeletalMeshRenderData* InSkelMeshRenderData)
	: FPrimitiveSceneProxy(Component)
	, MeshObject(Component->MeshObject)
	, SkeletalMeshRenderData(InSkelMeshRenderData)
{
	bool bCastShadow = Component->CastShadow;
	bool bAnySectionCastsShadow = false;
	LODSections.reserve(SkeletalMeshRenderData->LODRenderData.size());
	LODSections.resize(SkeletalMeshRenderData->LODRenderData.size());
	for (uint32 LODIdx = 0; LODIdx < SkeletalMeshRenderData->LODRenderData.size(); LODIdx++)
	{
		const FSkeletalMeshLODRenderData& LODData = *SkeletalMeshRenderData->LODRenderData[LODIdx];
		const FSkeletalMeshLODInfo& Info = *(Component->SkeletalMesh->GetLODInfo(LODIdx));

		FLODSectionElements& LODSection = LODSections[LODIdx];

		LODSection.SectionElements.clear();
		LODSection.SectionElements.reserve(LODData.RenderSections.size());

		for (uint32 SectionIndex = 0; SectionIndex < LODData.RenderSections.size(); SectionIndex++)
		{
			const FSkeletalMeshRenderSection& Section = LODData.RenderSections[SectionIndex];

			int32 UseMaterialIndex = Section.MaterialIndex;

			bool bSectionHidden = false;// MeshObject->IsMaterialHidden(LODIdx, UseMaterialIndex);

										// If the material is NULL, or isn't flagged for use with skeletal meshes, it will be replaced by the default material.
			UMaterial* Material = Component->GetMaterial(UseMaterialIndex);

			bool bSectionCastsShadow = !bSectionHidden && bCastShadow &&
				(IsValidIndex(Component->SkeletalMesh->Materials,UseMaterialIndex) == false || Section.bCastShadow);

			bAnySectionCastsShadow |= bSectionCastsShadow;
			LODSection.SectionElements.push_back(
				FSectionElementInfo(
					Material,
					bSectionCastsShadow,
					UseMaterialIndex
				));
		}
	}

	bCastDynamicShadow = bCastDynamicShadow && bAnySectionCastsShadow;
}

void FSkeletalMeshSceneProxy::GetDynamicMeshElements(const std::vector<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	GetMeshElementsConditionallySelectable(Views, ViewFamily, true, VisibilityMap, Collector);
}

FPrimitiveViewRelevance FSkeletalMeshSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = true;// IsShown(View) && View->Family->EngineShowFlags.SkeletalMeshes;
	Result.bShadowRelevance = true;//IsShadowCast(View);
	Result.bStaticRelevance = false;//bRenderStatic && !IsRichView(*View->Family);
	Result.bDynamicRelevance = true;//!Result.bStaticRelevance;
	Result.bRenderCustomDepth = true;//ShouldRenderCustomDepth();
	Result.bRenderInMainPass = true;//ShouldRenderInMainPass();
	Result.bUsesLightingChannels = true;//GetLightingChannelMask() != GetDefaultLightingChannelMask();

	//MaterialRelevance.SetPrimitiveViewRelevance(Result);

	return Result;
}

void FSkeletalMeshSceneProxy::GetMeshElementsConditionallySelectable(const std::vector<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, bool bInSelectable, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	if (!MeshObject)
	{
		return;
	}
	//MeshObject->PreGDMECallback(ViewFamily.Scene->GetGPUSkinCache(), ViewFamily.FrameNumber);

	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];
			//MeshObject->UpdateMinDesiredLODLevel(View, GetBounds(), ViewFamily.FrameNumber);
		}
	}

	//const FEngineShowFlags& EngineShowFlags = ViewFamily.EngineShowFlags;

	const int32 LODIndex = MeshObject->GetLOD();
	assert(LODIndex < (int32)SkeletalMeshRenderData->LODRenderData.size());
	const FSkeletalMeshLODRenderData& LODData = *SkeletalMeshRenderData->LODRenderData[LODIndex];

	if (LODSections.size() > 0)
	{
		const FLODSectionElements& LODSection = LODSections[LODIndex];

		assert(LODSection.SectionElements.size() == LODData.RenderSections.size());

		for (uint32 SectionIndex = 0; SectionIndex < LODSection.SectionElements.size();++SectionIndex)
		{
			const FSkeletalMeshRenderSection& Section = LODData.RenderSections[SectionIndex];
			//const int32 SectionIndex = Iter.GetSectionElementIndex();
			//const FSectionElementInfo& SectionElementInfo = Iter.GetSectionElementInfo();
			const FSectionElementInfo& SectionElementInfo = LODSection.SectionElements[SectionIndex];

			bool bSectionSelected = false;

			// If hidden skip the draw
// 			if (MeshObject->IsMaterialHidden(LODIndex, SectionElementInfo.UseMaterialIndex) || Section.bDisabled)
// 			{
// 				continue;
// 			}

			GetDynamicElementsSection(Views, ViewFamily, VisibilityMap, LODData, LODIndex, SectionIndex, bSectionSelected, SectionElementInfo, bInSelectable, Collector);
		}
	}
}


void FSkeletalMeshSceneProxy::GetDynamicElementsSection(
	const std::vector<const FSceneView*>& Views, 
	const FSceneViewFamily& ViewFamily,
	uint32 VisibilityMap, 
	const FSkeletalMeshLODRenderData& LODData, 
	const int32 LODIndex, 
	const int32 SectionIndex, 
	bool bSectionSelected, 
	const FSectionElementInfo& SectionElementInfo, 
	bool bInSelectable, 
	FMeshElementCollector& Collector) const
{
	const FSkeletalMeshRenderSection& Section = LODData.RenderSections[SectionIndex];

	const bool bIsSelected = false;

	const bool bIsWireframe = false;// ViewFamily.EngineShowFlags.Wireframe;

	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];

			FMeshBatch& Mesh = Collector.AllocateMesh();
			FMeshBatchElement& BatchElement = Mesh.Elements[0];
			Mesh.LCI = NULL;
			Mesh.bWireframe |= false/*bForceWireframe*/;
			Mesh.Type = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;// PT_TriangleList;
			Mesh.VertexFactory = MeshObject->GetSkinVertexFactory(View, LODIndex, SectionIndex);

			if (!Mesh.VertexFactory)
			{
				// hide this part
				continue;
			}

			Mesh.bSelectable = bInSelectable;
			BatchElement.FirstIndex = Section.BaseIndex;

			BatchElement.IndexBuffer = LODData.MultiSizeIndexContainer.GetIndexBuffer();
			BatchElement.MaxVertexIndex = LODData.GetNumVertices() - 1;
			//BatchElement.VertexFactoryUserData = FGPUSkinCache::GetFactoryUserData(MeshObject->SkinCacheEntry, SectionIndex);

			Mesh.MaterialRenderProxy = SectionElementInfo.Material->GetRenderProxy(false, false/*IsHovered()*/);

			BatchElement.PrimitiveUniformBufferResource = &GetUniformBuffer();

			BatchElement.NumPrimitives = Section.NumTriangles;

			BatchElement.MinVertexIndex = Section.BaseVertexIndex;

			Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
			Mesh.CastShadow = SectionElementInfo.bEnableShadowCasting;

			Mesh.bCanApplyViewModeOverrides = true;
			Mesh.bUseWireframeSelectionColoring = bIsSelected;

			Collector.AddMesh(ViewIndex, Mesh);
		}
	}
}
