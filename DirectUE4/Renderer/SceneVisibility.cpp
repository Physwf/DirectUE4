#include "DeferredShading.h"
#include "Scene.h"

float GLightMaxDrawDistanceScale = 1.0f;
float GMinScreenRadiusForLights = 0.03f;
float GMinScreenRadiusForDepthPrepass = 0.03f;

void FSceneRenderer::PreVisibilityFrameSetup()
{

}

void FSceneRenderer::ComputeViewVisibility()
{
	uint32 NumPrimitives = Scene->Primitives.size();

	FPrimitiveViewMasks HasDynamicMeshElementsMasks;
	HasDynamicMeshElementsMasks.resize(NumPrimitives);

	FPrimitiveViewMasks HasViewCustomDataMasks;
	HasViewCustomDataMasks.resize(NumPrimitives);

	FPrimitiveViewMasks HasDynamicEditorMeshElementsMasks;

	if (Scene->Lights.size() > 0)
	{
		VisibleLightInfos.resize(Scene->Lights.size());
	}

	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ++ViewIndex)
	{
		FViewInfo& View = Views[ViewIndex];

		View.VisibleLightInfos.reserve(Scene->Lights.size());

		for (uint32 LightIndex = 0; LightIndex < Scene->Lights.size(); LightIndex++)
		{
			View.VisibleLightInfos.push_back(FVisibleLightViewInfo());
		}
	}

	GatherDynamicMeshElements(Views, Scene, ViewFamily, HasDynamicMeshElementsMasks, HasDynamicEditorMeshElementsMasks, HasViewCustomDataMasks, MeshCollector);
}

void FSceneRenderer::PostVisibilityFrameSetup()
{
	Scene->IndirectLightingCache.UpdateCache(Scene, *this, true);


	InitFogConstants();
}


void FSceneRenderer::InitViewsPossiblyAfterPrepass()
{
	InitDynamicShadows();

	UpdatePrimitivePrecomputedLightingBuffers();
}

void FSceneRenderer::GatherDynamicMeshElements(
	std::vector<FViewInfo>& InViews, 
	const FScene* InScene, 
	const FSceneViewFamily& InViewFamily, 
	const FPrimitiveViewMasks& HasDynamicMeshElementsMasks, 
	const FPrimitiveViewMasks& HasDynamicEditorMeshElementsMasks, 
	const FPrimitiveViewMasks& HasViewCustomDataMasks, 
	FMeshElementCollector& Collector)
{
	uint32 NumPrimitives = InScene->Primitives.size();
	assert(HasDynamicMeshElementsMasks.size() == NumPrimitives);

	int32 ViewCount = InViews.size();
	{
		Collector.ClearViewMeshArrays();

		for (int32 ViewIndex = 0; ViewIndex < ViewCount; ViewIndex++)
		{
			Collector.AddViewMeshArrays(&InViews[ViewIndex], &InViews[ViewIndex].DynamicMeshElements /*,&InViews[ViewIndex].SimpleElementCollector*//*, InViewFamily.GetFeatureLevel()*/);
		}

		const bool bIsInstancedStereo = false;// (ViewCount > 0) ? (InViews[0].IsInstancedStereoPass() || InViews[0].bIsMobileMultiViewEnabled) : false;

		for (uint32 PrimitiveIndex = 0; PrimitiveIndex < NumPrimitives; ++PrimitiveIndex)
		{
			const uint8 ViewMask = HasDynamicMeshElementsMasks[PrimitiveIndex];

			if (ViewMask != 0)
			{
				// Don't cull a single eye when drawing a stereo pair
				const uint8 ViewMaskFinal = (bIsInstancedStereo) ? ViewMask | 0x3 : ViewMask;

				FPrimitiveSceneInfo* PrimitiveSceneInfo = InScene->Primitives[PrimitiveIndex];
				//Collector.SetPrimitive(PrimitiveSceneInfo->Proxy, PrimitiveSceneInfo->DefaultDynamicHitProxyId);

				//SetDynamicMeshElementViewCustomData(InViews, HasViewCustomDataMasks, PrimitiveSceneInfo);

				PrimitiveSceneInfo->Proxy->GetDynamicMeshElements(InViewFamily.Views, InViewFamily, ViewMaskFinal, Collector);
			}

			// to support GetDynamicMeshElementRange()
			for (int32 ViewIndex = 0; ViewIndex < ViewCount; ViewIndex++)
			{
				InViews[ViewIndex].DynamicMeshEndIndices[PrimitiveIndex] = Collector.GetMeshBatchCount(ViewIndex);
			}
		}
	}
}

void FSceneRenderer::InitViews()
{
	PreVisibilityFrameSetup();

	ComputeViewVisibility();

	PostVisibilityFrameSetup();

	InitViewsPossiblyAfterPrepass();

	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		FViewInfo& View = Views[ViewIndex];
		// Initialize the view's RHI resources.
		View.InitRHIResources();
	}

	for (FPrimitiveSceneInfo* Primitive : Scene->Primitives)
	{
		Primitive->ConditionalUpdateUniformBuffer();
	}
}

