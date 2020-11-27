#include "DeferredShading.h"
#include "ShadowRendering.h"
#include "Scene.h"

void FProjectedShadowInfo::CopyCachedShadowMap(const FDrawingPolicyRenderState& DrawRenderState, FSceneRenderer* SceneRenderer, const FViewInfo& View, FSetShadowRenderTargetFunction SetShadowRenderTargets)
{

}

void FProjectedShadowInfo::RenderDepthInner(class FSceneRenderer* SceneRenderer, const FViewInfo* FoundView, FSetShadowRenderTargetFunction SetShadowRenderTargets, EShadowDepthRenderMode RenderMode)
{

}

void FProjectedShadowInfo::RenderDepthDynamic(class FSceneRenderer* SceneRenderer, const FViewInfo* FoundView, const FDrawingPolicyRenderState& DrawRenderState)
{

}

void FProjectedShadowInfo::RenderDepth(class FSceneRenderer* SceneRenderer, FSetShadowRenderTargetFunction SetShadowRenderTargets, EShadowDepthRenderMode RenderMode)
{

}

void FSceneRenderer::RenderShadowDepthMapAtlases()
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();


}

void FSceneRenderer::RenderShadowDepthMaps()
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

	FSceneRenderer::RenderShadowDepthMapAtlases();

	for (uint32 CubemapIndex = 0; CubemapIndex < SortedShadowsForShadowDepthPass.ShadowMapCubemaps.size(); CubemapIndex++)
	{
		const FSortedShadowMapAtlas& ShadowMap = SortedShadowsForShadowDepthPass.ShadowMapCubemaps[CubemapIndex];
		PooledRenderTarget& RenderTarget = *ShadowMap.RenderTargets.DepthTarget.Get();
		FIntPoint TargetSize = ShadowMap.RenderTargets.DepthTarget->GetDesc().Extent;

		assert(ShadowMap.Shadows.size() == 1);
		FProjectedShadowInfo* ProjectedShadowInfo = ShadowMap.Shadows[0];

		auto SetShadowRenderTargets = [this, &RenderTarget, &SceneContext](bool bPerformClear)
		{
			SetRenderTarget(nullptr, RenderTarget.TargetableTexture.get(), false, true, false);
		};

		{
			bool bDoClear = true;

			if (ProjectedShadowInfo->CacheMode == SDCM_MovablePrimitivesOnly && Scene->CachedShadowMaps.at(ProjectedShadowInfo->GetLightSceneInfo().Id).bCachedShadowMapHasPrimitives)
			{
				// Skip the clear when we'll copy from a cached shadowmap
				bDoClear = false;
			}

			//SCOPED_CONDITIONAL_DRAW_EVENT(RHICmdList, Clear, bDoClear);
			SetShadowRenderTargets(bDoClear);
		}

		ProjectedShadowInfo->RenderDepth(this, SetShadowRenderTargets, ShadowDepthRenderMode_Normal);

		//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, RenderTarget.TargetableTexture);
	}
}
