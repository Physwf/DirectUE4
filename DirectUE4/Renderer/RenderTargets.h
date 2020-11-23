#pragma once

#include "D3D11RHI.h"
#include "UnrealMath.h"
#include "RenderTargetPool.h"

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

class SceneRenderer;
class SceneViewFamily;

class RenderTargets
{
public:
	static RenderTargets& Get();

	void BeginRenderingSceneColor();

	void BeginRenderingPrePass(bool bClear);
	void FinishRenderingPrePass();

	void BeginRenderingGBuffer(bool bClearColor, const FLinearColor& ClearColor = { 0,0,0,1 });
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

	//void AllocateReflectionTargets(FRHICommandList& RHICmdList, int32 TargetSize);

	//void AllocateLightingChannelTexture(FRHICommandList& RHICmdList);

	//void AllocateDebugViewModeTargets(FRHICommandList& RHICmdList);

	//void AllocateScreenShadowMask(FRHICommandList& RHICmdList, TRefCountPtr<IPooledRenderTarget>& ScreenShadowMaskTexture);
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

	void Allocate(const SceneRenderer* Renderer);
private:
	void AllocRenderTargets();
	void AllocateDeferredShadingPathRenderTargets();
	void AllocSceneColor();
	void AllocateCommonDepthTargets();
	void AllocLightAttenuation();

	const ComPtr<PooledRenderTarget>& GetSceneColorForCurrentShadingPath() const {  return SceneColor; }
	ComPtr<PooledRenderTarget>& GetSceneColorForCurrentShadingPath() { return SceneColor; }

	FIntPoint ComputeDesiredSize(const SceneViewFamily& ViewFamily);

	int32 GetGBufferRenderTargets(ERenderTargetLoadAction ColorLoadAction, FD3D11Texture2D* OutRenderTargets[8], int32& OutVelocityRTIndex);

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
	//ComPtr<PooledRenderTarget> GBufferD;
	//ComPtr<PooledRenderTarget> GBufferE;

	//ComPtr<PooledRenderTarget> GBufferVelocity;

	ComPtr<PooledRenderTarget> ScreenSpaceAO;
	//ComPtr<PooledRenderTarget> CustomDepth;
	//ComPtr<PooledRenderTarget> MobileCustomStencil;
	//ComPtr<PooledRenderTarget> CustomStencilSRV;

	/** To detect a change of the CVar r.MobileMSAA or r.MSAA */

	FIntPoint BufferSize;


	int32 CurrentMSAACount;

	bool bScreenSpaceAOIsValid;

	FClearValueBinding DefaultColorClear;
	FClearValueBinding DefaultDepthClear;

};

struct FResolveRect
{
	int32 X1;
	int32 Y1;
	int32 X2;
	int32 Y2;
	// e.g. for a a full 256 x 256 area starting at (0, 0) it would be 
	// the values would be 0, 0, 256, 256
	inline FResolveRect(int32 InX1 = -1, int32 InY1 = -1, int32 InX2 = -1, int32 InY2 = -1)
		: X1(InX1)
		, Y1(InY1)
		, X2(InX2)
		, Y2(InY2)
	{}

	inline FResolveRect(const FResolveRect& Other)
		: X1(Other.X1)
		, Y1(Other.Y1)
		, X2(Other.X2)
		, Y2(Other.Y2)
	{}

	bool IsValid() const
	{
		return X1 >= 0 && Y1 >= 0 && X2 - X1 > 0 && Y2 - Y1 > 0;
	}
};

struct FResolveParams
{
	/** used to specify face when resolving to a cube map texture */
	ECubeFace CubeFace;
	/** resolve RECT bounded by [X1,Y1]..[X2,Y2]. Or -1 for fullscreen */
	FResolveRect Rect;
	FResolveRect DestRect;
	/** The mip index to resolve in both source and dest. */
	int32 MipIndex;
	/** Array index to resolve in the source. */
	int32 SourceArrayIndex;
	/** Array index to resolve in the dest. */
	int32 DestArrayIndex;

	/** constructor */
	FResolveParams(
		const FResolveRect& InRect = FResolveRect(),
		ECubeFace InCubeFace = CubeFace_PosX,
		int32 InMipIndex = 0,
		int32 InSourceArrayIndex = 0,
		int32 InDestArrayIndex = 0,
		const FResolveRect& InDestRect = FResolveRect())
		: CubeFace(InCubeFace)
		, Rect(InRect)
		, DestRect(InDestRect)
		, MipIndex(InMipIndex)
		, SourceArrayIndex(InSourceArrayIndex)
		, DestArrayIndex(InDestArrayIndex)
	{}

	inline FResolveParams(const FResolveParams& Other)
		: CubeFace(Other.CubeFace)
		, Rect(Other.Rect)
		, DestRect(Other.DestRect)
		, MipIndex(Other.MipIndex)
		, SourceArrayIndex(Other.SourceArrayIndex)
		, DestArrayIndex(Other.DestArrayIndex)
	{}
};

void CopyToResolveTarget(FD3D11Texture2D* SourceTextureRHI, FD3D11Texture2D* DestTextureRHI, const FResolveParams& ResolveParams);