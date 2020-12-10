#include "SkeletalMesh.h"
#include "log.h"
#include "SkeletalMeshTypes.h"
#include "MeshComponent.h"
#include "SkeletalRenderGPUSkin.h"

USkeletalMesh::USkeletalMesh(class AActor* InOwner)
{
	ImportedModel = std::make_unique<SkeletalMeshModel>();
}

void USkeletalMesh::CalculateRequiredBones(SkeletalMeshLODModel& LODModel, const struct FReferenceSkeleton& RefSkeleton, const std::map<FBoneIndexType, FBoneIndexType>* BonesToRemove)
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


void USkeletalMesh::CacheDerivedData()
{
	AllocateResourceForRendering();
	RenderdData->Cache(this);
	//PostMeshCached.Broadcast(this);
}

FSkeletalMeshSceneProxy::FSkeletalMeshSceneProxy(const USkinnedMeshComponent* Component, FSkeletalMeshRenderData* InSkelMeshRenderData)
	: FPrimitiveSceneProxy(Component)
{

}

void FSkeletalMeshSceneProxy::GetDynamicMeshElements(const std::vector<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	GetMeshElementsConditionallySelectable(Views, ViewFamily, true, VisibilityMap, Collector);
}

FPrimitiveViewRelevance FSkeletalMeshSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	return FPrimitiveViewRelevance();
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
		}
	}
}
