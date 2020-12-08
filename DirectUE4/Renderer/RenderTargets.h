#pragma once

#include "D3D11RHI.h"
#include "UnrealMath.h"
#include "RenderTargetPool.h"

/*
* Stencil layout during basepass / deferred decals:
*		BIT ID    | USE
*		[0]       | sandbox bit (bit to be use by any rendering passes, but must be properly reset to 0 after using)
*		[1]       | unallocated
*		[2]       | unallocated
*		[3]       | Temporal AA mask for translucent object.
*		[4]       | Lighting channels
*		[5]       | Lighting channels
*		[6]       | Lighting channels
*		[7]       | primitive receive decal bit
*
* After deferred decals, stencil is cleared to 0 and no longer packed in this way, to ensure use of fast hardware clears and HiStencil.
*/
#define STENCIL_SANDBOX_BIT_ID				0
#define STENCIL_TEMPORAL_RESPONSIVE_AA_BIT_ID 3
#define STENCIL_LIGHTING_CHANNELS_BIT_ID	4
#define STENCIL_RECEIVE_DECAL_BIT_ID		7

// Outputs a compile-time constant stencil's bit mask ready to be used
// in TStaticDepthStencilState<> template parameter. It also takes care
// of masking the Value macro parameter to only keep the low significant
// bit to ensure to not overflow on other bits.
#define GET_STENCIL_BIT_MASK(BIT_NAME,Value) uint8((uint8(Value) & uint8(0x01)) << (STENCIL_##BIT_NAME##_BIT_ID))

#define STENCIL_SANDBOX_MASK GET_STENCIL_BIT_MASK(SANDBOX,1)

#define STENCIL_TEMPORAL_RESPONSIVE_AA_MASK GET_STENCIL_BIT_MASK(TEMPORAL_RESPONSIVE_AA,1)

#define STENCIL_LIGHTING_CHANNELS_MASK(Value) uint8((Value & 0x7) << STENCIL_LIGHTING_CHANNELS_BIT_ID)

namespace EGBufferFormat
{
	// When this enum is updated please update CVarGBufferFormat comments 
	enum Type
	{
		Force8BitsPerChannel = 0 ,
		Default = 1 ,
		HighPrecisionNormals = 3 ,
		Force16BitsPerChannel = 5 ,
	};
}

static const int32 NumCubeShadowDepthSurfaces = 5;

class FSceneRenderer;
class FSceneViewFamily;

class FSceneRenderTargets
{
public:
	static FSceneRenderTargets& Get();

	void BeginRenderingSceneColor(bool bClearColor,bool bClearDepth, bool bClearStencil);

	void BeginRenderingPrePass(bool bClear);
	void FinishRenderingPrePass();

	void BeginRenderingGBuffer(ERenderTargetLoadAction ColorLoadAction, ERenderTargetLoadAction DepthLoadAction, FExclusiveDepthStencil::Type DepthStencilAccess, const FLinearColor& ClearColor = { 0,0,0,1 });
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

	void SetDefaultColorClear(const FClearValueBinding ColorClear)
	{
		DefaultColorClear = ColorClear;
	}

	void SetDefaultDepthClear(const FClearValueBinding DepthClear)
	{
		DefaultDepthClear = DepthClear;
	}

	FClearValueBinding GetDefaultDepthClear()
	{
		return DefaultDepthClear;
	}

	void FinishRendering();

	int32 GetMSAACount() const { return CurrentMSAACount; }

	EPixelFormat GetSceneColorFormat() const;

	void SetBufferSize(int32 InBufferSizeX, int32 InBufferSizeY);
	/** Returns the size of most screen space render targets e.g. SceneColor, SceneDepth, GBuffer, ... might be different from final RT or output Size because of ScreenPercentage use. */
	FIntPoint GetBufferSizeXY() const { return BufferSize; }

	//void PreallocGBufferTargets();
	void GetGBufferADesc(PooledRenderTargetDesc& Desc) const;
	void AllocGBufferTargets();

	void AllocateReflectionTargets(int32 TargetSize);

	//void AllocateLightingChannelTexture(FRHICommandList& RHICmdList);

	//void AllocateDebugViewModeTargets(FRHICommandList& RHICmdList);

	void AllocateScreenShadowMask(ComPtr<PooledRenderTarget>& ScreenShadowMaskTexture);


	FIntPoint GetShadowDepthTextureResolution() const;
	int32 GetCubeShadowDepthZIndex(int32 ShadowResolution) const;
	/** Returns the appropriate resolution for a given cube shadow index. */
	int32 GetCubeShadowDepthZResolution(int32 ShadowIndex) const;
public:
	const std::shared_ptr<FD3D11Texture2D>& GetSceneColorTexture() const;
	const std::shared_ptr<FD3D11Texture2D>& GetSceneDepthTexture() const { return SceneDepthZ->ShaderResourceTexture; }

	const std::shared_ptr<FD3D11Texture2D>* GetActualDepthTexture() const;
	const std::shared_ptr<FD3D11Texture2D>& GetGBufferATexture() const { return GBufferA->ShaderResourceTexture; }
	const std::shared_ptr<FD3D11Texture2D>& GetGBufferBTexture() const { return GBufferB->ShaderResourceTexture; }
	const std::shared_ptr<FD3D11Texture2D>& GetGBufferCTexture() const { return GBufferC->ShaderResourceTexture; }
	//const std::shared_ptr<FD3D11Texture2D>& GetGBufferDTexture() const { return GBufferD->ShaderResourceTexture; }
	//const std::shared_ptr<FD3D11Texture2D>& GetGBufferETexture() const { return GBufferE->ShaderResourceTexture; }
	//const std::shared_ptr<FD3D11Texture2D>& GetGBufferVelocityTexture() const { return GBufferVelocity->ShaderResourceTexture; }

	const std::shared_ptr<FD3D11Texture2D>& GetLightAttenuationTexture() const { LightAttenuation->ShaderResourceTexture; }

	const std::shared_ptr<FD3D11Texture2D>& GetSceneColorSurface() const { return SceneColor->TargetableTexture; }
	//const std::shared_ptr<FD3D11Texture2D>& GetSceneAlphaCopySurface() const { return (const FTexture2DRHIRef&)SceneAlphaCopy->GetRenderTargetItem().TargetableTexture; }
	const std::shared_ptr<FD3D11Texture2D>& GetSceneDepthSurface() const { return SceneDepthZ->TargetableTexture; }
	//const std::shared_ptr<FD3D11Texture2D>& GetSmallDepthSurface() const { return (const FTexture2DRHIRef&)SmallDepthZ->GetRenderTargetItem().TargetableTexture; }
	//const std::shared_ptr<FD3D11Texture2D>& GetOptionalShadowDepthColorSurface(FRHICommandList& RHICmdList, int32 Width, int32 Height) const;
	const std::shared_ptr<FD3D11Texture2D>& GetLightAttenuationSurface() const { return LightAttenuation->TargetableTexture; }

	const std::shared_ptr<FD3D11Texture2D>& GetScreenSpaceAO() { return ScreenSpaceAO->ShaderResourceTexture; }

	const ComPtr<PooledRenderTarget>& GetSceneColor() const;

	ComPtr<PooledRenderTarget>& GetSceneColor();

	void Allocate(const FSceneRenderer* Renderer);

	void ReleaseSceneColor();
private:
	void AllocRenderTargets();
	void AllocateDeferredShadingPathRenderTargets();
	void AllocSceneColor();
	void AllocateCommonDepthTargets();
	void AllocLightAttenuation();


	void ReleaseGBufferTargets();

	void ReleaseAllTargets();

	const ComPtr<PooledRenderTarget>& GetSceneColorForCurrentShadingPath() const {  return SceneColor; }
	ComPtr<PooledRenderTarget>& GetSceneColorForCurrentShadingPath() { return SceneColor; }

	FIntPoint ComputeDesiredSize(const FSceneViewFamily& ViewFamily);

	int32 GetGBufferRenderTargets(ERenderTargetLoadAction ColorLoadAction, FD3D11Texture2D* OutRenderTargets[8], int32& OutVelocityRTIndex);

	int32 CurrentMaxShadowResolution;

	int32 CurrentGBufferFormat;
	int32 CurrentSceneColorFormat;
	bool bAllowStaticLighting;
private:
	ComPtr<PooledRenderTarget> SceneColor;

	ComPtr<PooledRenderTarget> LightAttenuation;
public:
	ComPtr<PooledRenderTarget> LightAccumulation;
	ComPtr<PooledRenderTarget> DirectionalOcclusion;

	ComPtr<PooledRenderTarget> SceneDepthZ;
	//ComPtr<FRHIShaderResourceView> SceneStencilSRV;
	ComPtr<PooledRenderTarget> LightingChannels;

	ComPtr<PooledRenderTarget> SceneAlphaCopy;
	// Auxiliary scene depth target. The scene depth is resolved to this surface when targeting SM4. 
	ComPtr<PooledRenderTarget> AuxiliarySceneDepthZ;
	// Quarter-sized version of the scene depths
	ComPtr<PooledRenderTarget> SmallDepthZ;


	ComPtr<PooledRenderTarget> GBufferA;
	ComPtr<PooledRenderTarget> GBufferB;
	ComPtr<PooledRenderTarget> GBufferC;
	ComPtr<PooledRenderTarget> GBufferD;
	ComPtr<PooledRenderTarget> GBufferE;

	//ComPtr<PooledRenderTarget> GBufferVelocity;

	ComPtr<PooledRenderTarget> ScreenSpaceAO;
	//ComPtr<PooledRenderTarget> CustomDepth;
	//ComPtr<PooledRenderTarget> MobileCustomStencil;
	//ComPtr<PooledRenderTarget> CustomStencilSRV;


	ComPtr<PooledRenderTarget> ReflectionColorScratchCubemap[2];
	/** Temporary storage during SH irradiance map generation. */
	ComPtr<PooledRenderTarget> DiffuseIrradianceScratchCubemap[2];
	/** Temporary storage during SH irradiance map generation. */
	ComPtr<PooledRenderTarget> SkySHIrradianceMap;

	/** To detect a change of the CVar r.MobileMSAA or r.MSAA */

	FIntPoint BufferSize;


	int32 CurrentMSAACount;

	bool bScreenSpaceAOIsValid;

	FClearValueBinding DefaultColorClear;
	FClearValueBinding DefaultDepthClear;

};

