#include "DeferredShading.h"
#include "Scene.h"
#include "DepthOnlyRendering.h"
#include "SceneOcclusion.h"
#include "BasePassRendering.h"
#include "ShadowRendering.h"
#include "LightRendering.h"
#include "AtmosphereRendering.h"
#include "RenderTargets.h"
#include "SceneFilterRendering.h"
#include "CompositionLighting.h"

#include <new>

ID3D11Buffer* GlobalConstantBuffer;
char GlobalConstantBufferData[4096];

Scene* GScene;

void InitShading()
{
	GlobalConstantBuffer = CreateConstantBuffer(false,4096);
	memset(GlobalConstantBufferData, 0, sizeof(GlobalConstantBufferData));

	RenderTargets& SceneContex = RenderTargets::Get();
	SceneContex.Allocate();
	SceneContex.SetBufferSize(int32(WindowWidth), int32(WindowHeight));

	InitScreenRectangleResources();

	InitPrePass();
	InitHZB();
	InitShadowDepthMapsPass();
	InitBasePass();
	GCompositionLighting.Init();
	InitLightPass();
	InitAtomosphereFog();
}

SceneRenderer::SceneRenderer(SceneViewFamily& InViewFamily)
	:mScene(InViewFamily.mScene),
	ViewFamily(InViewFamily)
{
	for (uint32 ViewIndex = 0; ViewIndex < InViewFamily.Views.size(); ViewIndex++)
	{
		ViewInfo View(InViewFamily.Views[ViewIndex]);
		Views.emplace_back(View);
	}
}

void SceneRenderer::PrepareViewRectsForRendering()
{
	for (ViewInfo& View : Views)
	{
		View.ViewRect = View.UnscaledViewRect;
	}
}

void SceneRenderer::InitViews()
{
	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		ViewInfo& View = Views[ViewIndex];
		// Initialize the view's RHI resources.
		View.InitRHIResources();
	}
}
void SceneRenderer::Render()
{
	PrepareViewRectsForRendering();

	InitViews();
	RenderPrePass();
	RenderHzb();
	RenderShadowDepthMaps();
	RenderBasePass();
	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		GCompositionLighting.ProcessAfterBasePass(Views[ViewIndex]);
	}
	RenderLights();
	RenderAtmosphereFog();

	RenderTargets& SceneContex = RenderTargets::Get();
	SceneContex.FinishRendering();
}

