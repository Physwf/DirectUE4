#include "LightRendering.h"
#include "Scene.h"
#include "RenderTargets.h"
#include "DeferredShading.h"
#include "GPUProfiler.h"


void FSceneRenderer::RenderLight()
{

}

void FSceneRenderer::RenderLights()
{
	SCOPED_DRAW_EVENT_FORMAT(RenderLights, TEXT("Lights"));

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();
	SceneContext.BeginRenderingSceneColor();
	RenderLight();
}

