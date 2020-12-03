#include "DeferredShading.h"
#include "Scene.h"

void FSceneRenderer::PreVisibilityFrameSetup()
{

}

void FSceneRenderer::ComputeViewVisibility()
{
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
}

void FSceneRenderer::PostVisibilityFrameSetup()
{
	Scene->IndirectLightingCache.UpdateCache(Scene, *this, true);
}


void FSceneRenderer::InitViewsPossiblyAfterPrepass()
{
	InitDynamicShadows();

	UpdatePrimitivePrecomputedLightingBuffers();
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

