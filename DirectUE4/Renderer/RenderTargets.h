#pragma once

#include "D3D11RHI.h"
#include "UnrealMath.h"

class RenderTargets
{
public:
	static RenderTargets& Get();

	void BeginRenderingSceneColor();

	void BeginRenderingPrePass(bool bClear);
	void FinishRenderingPrePass();

	void BeginRenderingGBuffer(bool bClearColor, const LinearColor& ClearColor = { 0,0,0,1 });
	void FinishRenderingGBuffer();

	bool BeginRenderingCustomDepth();
	// only call if BeginRenderingCustomDepth() returned true
	void FinishRenderingCustomDepth();

	/** Binds the appropriate shadow depth cube map for rendering. */
	void BeginRenderingCubeShadowDepth();

	/** Begin rendering translucency in the scene color. */
	void BeginRenderingTranslucency();
	/** Begin rendering translucency in a separate (offscreen) buffer. This can be any translucency pass. */
	void BeginRenderingSeparateTranslucency();
	void FinishRenderingSeparateTranslucency();


	void FinishRendering();

	int32 GetMSAACount() const { return CurrentMSAACount; }

	void SetBufferSize(int32 InBufferSizeX, int32 InBufferSizeY);
	/** Returns the size of most screen space render targets e.g. SceneColor, SceneDepth, GBuffer, ... might be different from final RT or output Size because of ScreenPercentage use. */
	IntPoint GetBufferSizeXY() const { return BufferSize; }

public:
	const ID3D11Texture2D* GetSceneColorTexture() const { return SceneColorRT; }
	const ID3D11Texture2D* GetSceneDepthTexture() const { return SceneDepthRT; }

	const ID3D11Texture2D* GetGBufferATexture() const { return GBufferART; }
	const ID3D11Texture2D* GetGBufferBTexture() const { return GBufferBRT; }
	const ID3D11Texture2D* GetGBufferCTexture() const { return GBufferCRT; }
	const ID3D11Texture2D* GetGBufferDTexture() const { return GBufferDRT; }
	const ID3D11Texture2D* GetGBufferETexture() const { return GBufferERT; }
	const ID3D11Texture2D* GetGBufferVelocityTexture() const { return NULL; }

	ID3D11Texture2D* GetSceneColorTexture() { return SceneColorRT; }
	ID3D11Texture2D* GetSceneDepthTexture() { return SceneDepthRT; }

	ID3D11Texture2D* GetGBufferATexture() { return GBufferART; }
	ID3D11Texture2D* GetGBufferBTexture() { return GBufferBRT; }
	ID3D11Texture2D* GetGBufferCTexture() { return GBufferCRT; }
	ID3D11Texture2D* GetGBufferDTexture() { return GBufferDRT; }
	ID3D11Texture2D* GetGBufferETexture() { return GBufferERT; }
	ID3D11Texture2D* GetGBufferVelocityTexture() { return NULL; }

	ID3D11Texture2D* GetScreenSpaceAO() { return ScreenSpaceAO; }
	void Allocate();
private:
	void AllocRenderTargets();
	void AllocSceneColor();
	void AllocateCommonDepthTargets();
	void AllocGBuffer();
	void AllocLightAttenuation();

	int32 GetGBufferRenderTargets(ID3D11RenderTargetView* OutRenderTargets[8], int& OutVelocityRTIndex);
private:
	/** To detect a change of the CVar r.MobileMSAA or r.MSAA */
	int32 CurrentMSAACount;

	ID3D11Texture2D * SceneColorRT = NULL;
	ID3D11RenderTargetView* SceneColorRTV = NULL;
	ID3D11Texture2D* SceneDepthRT = NULL;
	ID3D11DepthStencilView* SceneDepthDSV = NULL;

	ID3D11DepthStencilView * ShadowPassDSV = NULL;
	
	ID3D11Texture2D* ShadowProjectionRT = NULL;
	ID3D11RenderTargetView* ShadowProjectionRTV = NULL;

	ID3D11Texture2D* GBufferART = NULL;
	ID3D11Texture2D* GBufferBRT = NULL;
	ID3D11Texture2D* GBufferCRT = NULL;
	ID3D11Texture2D* GBufferDRT = NULL;
	ID3D11Texture2D* GBufferERT = NULL;
	ID3D11RenderTargetView* GBufferARTV = NULL;
	ID3D11RenderTargetView* GBufferBRTV = NULL;
	ID3D11RenderTargetView* GBufferCRTV = NULL;
	ID3D11RenderTargetView* GBufferDRTV = NULL;
	ID3D11RenderTargetView* GBufferERTV = NULL;

	ID3D11Texture2D* ScreenSpaceAO;
	//G-Buffer
	ID3D11ShaderResourceView* GBufferSRV[6] = { NULL };
	ID3D11SamplerState* GBufferSamplerState[6] = { NULL };
	//Screen Space AO, Custom Depth, Custom Stencil
	ID3D11Texture2D* ScreenSapceAOTexture = NULL;
	ID3D11Texture2D* CustomDepthTexture = NULL;
	ID3D11Texture2D* CustomStencialTexture = NULL;

	IntPoint BufferSize;
};

