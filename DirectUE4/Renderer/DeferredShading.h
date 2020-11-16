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

	void PrepareViewRectsForRendering();
	void InitViews();
	void RenderPrePassView(FViewInfo& View, const FDrawingPolicyRenderState& DrawRenderState);
	void RenderPrePass();
	void RenderHzb();
	void RenderShadowDepthMaps();
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