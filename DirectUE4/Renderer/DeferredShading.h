#pragma once

#include "D3D11RHI.h"
#include "Scene.h"

extern ID3D11Buffer* GlobalConstantBuffer;
extern char GlobalConstantBufferData[4096];

void InitShading();

class SceneRenderer
{
public:
	SceneRenderer(SceneViewFamily& InViewFamily);

	void InitViewsPossiblyAfterPrepass();

	void InitDynamicShadows();

	void AllocateShadowDepthTargets();

// 	void AllocatePerObjectShadowDepthTargets(TArray<FProjectedShadowInfo*>& Shadows);
// 
// 	void AllocateCachedSpotlightShadowDepthTargets(TArray<FProjectedShadowInfo*>& CachedShadows);
// 
// 	void AllocateCSMDepthTargets(const TArray<FProjectedShadowInfo*>& WholeSceneDirectionalShadows);
// 
// 	void AllocateRSMDepthTargets(const TArray<FProjectedShadowInfo*>& RSMShadows);
// 
// 	void AllocateOnePassPointLightDepthTargets(const TArray<FProjectedShadowInfo*>& WholeScenePointShadows);
// 
// 	void AllocateTranslucentShadowDepthTargets(TArray<FProjectedShadowInfo*>& TranslucentShadows);

	void PrepareViewRectsForRendering();
	void InitViews();
	void RenderPrePassView(FViewInfo& View, const FDrawingPolicyRenderState& DrawRenderState);
	void RenderPrePass();
	void RenderHzb();
	void RenderShadowDepthMaps();
	void RenderShadowDepthMapAtlases();
	void RenderBasePass();
	void RenderLights();
	void RenderLight();
	void RenderAtmosphereFog();

	void Render();

	static FIntPoint GetDesiredInternalBufferSize(const SceneViewFamily& ViewFamily);
public:
	FScene* Scene;
	/** The view family being rendered.  This references the Views array. */
	SceneViewFamily ViewFamily;
	std::vector<FViewInfo> Views;
};