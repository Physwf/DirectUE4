#include "RenderTargets.h"
#include "DeferredShading.h"
#include "SystemTextures.h"
#include "SceneRenderTargetParameters.h"

FSceneRenderTargets& FSceneRenderTargets::Get()
{
	static FSceneRenderTargets Instance;
	return Instance;
}

void FSceneRenderTargets::BeginRenderingSceneColor(bool bClearColor, bool bClearDepth, bool bClearStencil)
{
	AllocSceneColor();
	SetRenderTarget(GetSceneColorSurface().get(), GetSceneDepthSurface().get(), bClearColor, bClearDepth, bClearStencil);
}

void FSceneRenderTargets::BeginRenderingPrePass(bool bClear)
{
	ID3D11DepthStencilView* DVS = GetSceneDepthSurface()->GetDepthStencilView(FExclusiveDepthStencil::DepthWrite);
	if (bClear)
	{
		float fClearDepth;
		UINT StencialClearValue;
		GetSceneDepthSurface()->GetDepthStencilClearValue(fClearDepth, StencialClearValue);
		D3D11DeviceContext->OMSetRenderTargets(0, NULL, DVS);
		D3D11DeviceContext->ClearDepthStencilView(DVS, D3D11_CLEAR_DEPTH, fClearDepth, StencialClearValue);
	}
	else
	{
		D3D11DeviceContext->OMSetRenderTargets(0, NULL, DVS);
	}
}

void FSceneRenderTargets::FinishRenderingPrePass()
{

}

void FSceneRenderTargets::BeginRenderingGBuffer(ERenderTargetLoadAction ColorLoadAction, ERenderTargetLoadAction DepthLoadAction, FExclusiveDepthStencil::Type DepthStencilAccess, const FLinearColor& ClearColor /*= (0, 0, 0, 1)*/)
{
	AllocSceneColor();

	FD3D11Texture2D* RenderTargets[8];

	bool bClearColor = ColorLoadAction == ERenderTargetLoadAction::EClear;
	bool bClearDepth = DepthLoadAction == ERenderTargetLoadAction::EClear;

	int32 VelocityRTIndex = -1;
	int32 MRTCount;

	MRTCount = GetGBufferRenderTargets(ColorLoadAction, RenderTargets, VelocityRTIndex);
	std::shared_ptr<FD3D11Texture2D> DepthView = GetSceneDepthSurface();
	SetRenderTargets(MRTCount, RenderTargets, DepthView.get());
}

void FSceneRenderTargets::FinishRenderingGBuffer()
{

}

void FSceneRenderTargets::FinishRendering()
{
	//D3D11DeviceContext->CopyResource(BackBuffer, SceneColorRT);
}

EPixelFormat FSceneRenderTargets::GetSceneColorFormat() const
{
	EPixelFormat SceneColorBufferFormat = PF_FloatRGBA;

// 	if (InFeatureLevel < ERHIFeatureLevel::SM4)
// 	{
// 		return GetMobileSceneColorFormat();
// 	}
// 	else
	{
		switch (CurrentSceneColorFormat)
		{
		case 0:
			SceneColorBufferFormat = PF_R8G8B8A8; break;
		case 1:
			SceneColorBufferFormat = PF_A2B10G10R10; break;
		case 2:
			SceneColorBufferFormat = PF_FloatR11G11B10; break;
		case 3:
			SceneColorBufferFormat = PF_FloatRGB; break;
		case 4:
			// default
			break;
		case 5:
			SceneColorBufferFormat = PF_A32B32G32R32F; break;
		}

		// Fallback in case the scene color selected isn't supported.
		if (!GPixelFormats[SceneColorBufferFormat].Supported)
		{
			SceneColorBufferFormat = PF_FloatRGBA;
		}

// 		if (bRequireSceneColorAlpha)
// 		{
// 			SceneColorBufferFormat = PF_FloatRGBA;
// 		}
	}

	return SceneColorBufferFormat;
}

void FSceneRenderTargets::SetBufferSize(int32 InBufferSizeX, int32 InBufferSizeY)
{
	QuantizeSceneBufferSize(FIntPoint(InBufferSizeX, InBufferSizeY), BufferSize);
}

void FSceneRenderTargets::Allocate(const FSceneRenderer* Renderer)
{
	FIntPoint DesiredBufferSize = ComputeDesiredSize(Renderer->ViewFamily);

	int GBufferFormat = 1; //CVarGBufferFormat.GetValueOnRenderThread();


	SetDefaultColorClear(FClearValueBinding::Black);
	SetDefaultDepthClear(FClearValueBinding::DepthFar);

	bool bNewAllowStaticLighting;
	{
		//static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowStaticLighting"));

		//bNewAllowStaticLighting = CVar->GetValueOnRenderThread() != 0;
		bNewAllowStaticLighting = true;
	}

	int SceneColorFormat;
	{
		//static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.SceneColorFormat"));

		//SceneColorFormat = CVar->GetValueOnRenderThread();
		SceneColorFormat = 4;
	}

	int32 MSAACount = 1;// GetNumSceneColorMSAASamples(NewFeatureLevel);

	int32 MaxShadowResolution = 2048;// GetCachedScalabilityCVars().MaxShadowResolution;

	CurrentGBufferFormat = GBufferFormat;
	CurrentSceneColorFormat = SceneColorFormat;
	bAllowStaticLighting = bNewAllowStaticLighting;
	//bUseDownsizedOcclusionQueries = bDownsampledOcclusionQueries;
	CurrentMaxShadowResolution = MaxShadowResolution;
	//CurrentRSMResolution = RSMResolution;
	//CurrentTranslucencyLightingVolumeDim = TranslucencyLightingVolumeDim;
	CurrentMSAACount = MSAACount;
	//CurrentMinShadowResolution = MinShadowResolution;
	//bCurrentLightPropagationVolume = bLightPropagationVolume;

	SetBufferSize(DesiredBufferSize.X,DesiredBufferSize.Y);

	AllocRenderTargets();
}

void FSceneRenderTargets::AllocRenderTargets()
{
	AllocateDeferredShadingPathRenderTargets();
}

void FSceneRenderTargets::AllocateDeferredShadingPathRenderTargets()
{
	AllocateCommonDepthTargets();

	{
		PooledRenderTargetDesc Desc(PooledRenderTargetDesc::Create2DDesc(BufferSize, PF_G8, FClearValueBinding::White, TexCreate_None, TexCreate_RenderTargetable, false));
		//Desc.Flags |= GFastVRamConfig.ScreenSpaceAO;

		//if (CurrentFeatureLevel >= ERHIFeatureLevel::SM5)
		{
			// UAV is only needed to support "r.AmbientOcclusion.Compute"
			// todo: ideally this should be only UAV or RT, not both
			Desc.TargetableFlags |= TexCreate_UAV;
		}
		GRenderTargetPool.FindFreeElement(Desc, ScreenSpaceAO, TEXT("ScreenSpaceAO"));
	}

	{
		PooledRenderTargetDesc Desc(PooledRenderTargetDesc::Create2DDesc(BufferSize, PF_FloatRGBA, FClearValueBinding::Black, TexCreate_None, TexCreate_RenderTargetable, false));
		//if (CurrentFeatureLevel >= ERHIFeatureLevel::SM5)
		{
			Desc.TargetableFlags |= TexCreate_UAV;
		}
		//Desc.Flags |= GFastVRamConfig.LightAccumulation;
		GRenderTargetPool.FindFreeElement(Desc, LightAccumulation, TEXT("LightAccumulation"));
	}


// 	if (bAllocateVelocityGBuffer)
// 	{
// 		FPooledRenderTargetDesc VelocityRTDesc = FVelocityRendering::GetRenderTargetDesc();
// 		VelocityRTDesc.Flags |= GFastVRamConfig.GBufferVelocity;
// 		GRenderTargetPool.FindFreeElement(RHICmdList, VelocityRTDesc, GBufferVelocity, TEXT("GBufferVelocity"));
// 	}

}

void FSceneRenderTargets::AllocSceneColor()
{
	ComPtr<PooledRenderTarget>& SceneColorTarget = GetSceneColorForCurrentShadingPath();
	if (SceneColorTarget &&
		SceneColorTarget->TargetableTexture->HasClearValue() &&
		!(SceneColorTarget->TargetableTexture->GetClearBinding() == DefaultColorClear))
	{
		const FLinearColor CurrentClearColor = SceneColorTarget->TargetableTexture->GetClearColor();
		const FLinearColor NewClearColor = DefaultColorClear.GetClearColor();
		SceneColorTarget.Reset();
	}

	if (GetSceneColorForCurrentShadingPath().Get())
	{
		return;
	}

	EPixelFormat SceneColorBufferFormat = GetSceneColorFormat();

	// Create the scene color.
	{
		PooledRenderTargetDesc Desc(PooledRenderTargetDesc::Create2DDesc(BufferSize, SceneColorBufferFormat, DefaultColorClear, TexCreate_None, TexCreate_RenderTargetable, false));
		//Desc.Flags |= GFastVRamConfig.SceneColor;
		Desc.NumSamples = 1;// GetNumSceneColorMSAASamples(CurrentFeatureLevel);

		//if (CurrentFeatureLevel >= ERHIFeatureLevel::SM5 && Desc.NumSamples == 1)
		{
			// GCNPerformanceTweets.pdf Tip 37: Warning: Causes additional synchronization between draw calls when using a render target allocated with this flag, use sparingly
			Desc.TargetableFlags |= TexCreate_UAV;
		}

		GRenderTargetPool.FindFreeElement(Desc, GetSceneColorForCurrentShadingPath(), TEXT("SceneColorDeferred"));
	}

	//check(GetSceneColorForCurrentShadingPath());
}

void FSceneRenderTargets::AllocateCommonDepthTargets()
{
	// Create a texture to store the resolved scene depth, and a render-targetable surface to hold the unresolved scene depth.
	PooledRenderTargetDesc Desc(PooledRenderTargetDesc::Create2DDesc(BufferSize, PF_DepthStencil, DefaultDepthClear, TexCreate_None, TexCreate_DepthStencilTargetable, false));
	Desc.NumSamples = 1;// GetNumSceneColorMSAASamples(CurrentFeatureLevel);
	//Desc.Flags |= GFastVRamConfig.SceneDepth;
	GRenderTargetPool.FindFreeElement(Desc, SceneDepthZ, TEXT("SceneDepthZ"));
}

void FSceneRenderTargets::GetGBufferADesc(PooledRenderTargetDesc& Desc) const
{
	// good to see the quality loss due to precision in the gbuffer
	const bool bHighPrecisionGBuffers = (CurrentGBufferFormat >= EGBufferFormat::Force16BitsPerChannel);
	// good to profile the impact of non 8 bit formats
	const bool bEnforce8BitPerChannel = (CurrentGBufferFormat == EGBufferFormat::Force8BitsPerChannel);

	// Create the world-space normal g-buffer.
	{
		EPixelFormat NormalGBufferFormat = bHighPrecisionGBuffers ? PF_FloatRGBA : PF_A2B10G10R10;

		if (bEnforce8BitPerChannel)
		{
			NormalGBufferFormat = PF_B8G8R8A8;
		}
		else if (CurrentGBufferFormat == EGBufferFormat::HighPrecisionNormals)
		{
			NormalGBufferFormat = PF_FloatRGBA;
		}

		Desc = PooledRenderTargetDesc::Create2DDesc(BufferSize, NormalGBufferFormat, FClearValueBinding::Transparent, TexCreate_None, TexCreate_RenderTargetable, false);
		//Desc.Flags |= GFastVRamConfig.GBufferA;
	}
}

void FSceneRenderTargets::AllocGBufferTargets()
{
	//ensure(GBufferRefCount == 0);

	if (GBufferA.Get())
	{
		// no work needed
		return;
	}

	// create GBuffer on demand so it can be shared with other pooled RT
	//const EShaderPlatform ShaderPlatform = GetFeatureLevelShaderPlatform(CurrentFeatureLevel);
	const bool bUseGBuffer = true;// IsUsingGBuffers(ShaderPlatform);
	const bool bCanReadGBufferUniforms = true;//(bUseGBuffer || IsSimpleForwardShadingEnabled(ShaderPlatform)) && CurrentFeatureLevel >= ERHIFeatureLevel::SM4;
	if (bUseGBuffer)
	{
		// good to see the quality loss due to precision in the gbuffer
		const bool bHighPrecisionGBuffers = (CurrentGBufferFormat >= EGBufferFormat::Force16BitsPerChannel);
		// good to profile the impact of non 8 bit formats
		const bool bEnforce8BitPerChannel = (CurrentGBufferFormat == EGBufferFormat::Force8BitsPerChannel);

		// Create the world-space normal g-buffer.
		{
			PooledRenderTargetDesc Desc;
			GetGBufferADesc(Desc);
			GRenderTargetPool.FindFreeElement(Desc, GBufferA, TEXT("GBufferA"));
		}

		// Create the specular color and power g-buffer.
		{
			const EPixelFormat SpecularGBufferFormat = bHighPrecisionGBuffers ? PF_FloatRGBA : PF_B8G8R8A8;

			PooledRenderTargetDesc Desc(PooledRenderTargetDesc::Create2DDesc(BufferSize, SpecularGBufferFormat, FClearValueBinding::Transparent, TexCreate_None, TexCreate_RenderTargetable, false));
			//Desc.Flags |= GFastVRamConfig.GBufferB;
			GRenderTargetPool.FindFreeElement(Desc, GBufferB, TEXT("GBufferB"));
		}

		// Create the diffuse color g-buffer.
		{
			const EPixelFormat DiffuseGBufferFormat = bHighPrecisionGBuffers ? PF_FloatRGBA : PF_B8G8R8A8;
			PooledRenderTargetDesc Desc(PooledRenderTargetDesc::Create2DDesc(BufferSize, DiffuseGBufferFormat, FClearValueBinding::Transparent, TexCreate_SRGB, TexCreate_RenderTargetable, false));
			//Desc.Flags |= GFastVRamConfig.GBufferC;
			GRenderTargetPool.FindFreeElement(Desc, GBufferC, TEXT("GBufferC"));
		}

		// Create the mask g-buffer (e.g. SSAO, subsurface scattering, wet surface mask, skylight mask, ...).
// 		{
// 			PooledRenderTargetDesc Desc(PooledRenderTargetDesc::Create2DDesc(BufferSize, PF_B8G8R8A8, FClearValueBinding::Transparent, TexCreate_None, TexCreate_RenderTargetable, false));
// 			//Desc.Flags |= GFastVRamConfig.GBufferD;
// 			GRenderTargetPool.FindFreeElement( Desc, GBufferD, TEXT("GBufferD"));
// 		}

// 		if (bAllowStaticLighting)
// 		{
// 			PooledRenderTargetDesc Desc(PooledRenderTargetDesc::Create2DDesc(BufferSize, PF_B8G8R8A8, FClearValueBinding::Transparent, TexCreate_None, TexCreate_RenderTargetable, false));
// 			//Desc.Flags |= GFastVRamConfig.GBufferE;
// 			GRenderTargetPool.FindFreeElement(Desc, GBufferE, TEXT("GBufferE"));
// 		}

// 		if (bAllocateVelocityGBuffer)
// 		{
// 			FPooledRenderTargetDesc VelocityRTDesc = FVelocityRendering::GetRenderTargetDesc();
// 			VelocityRTDesc.Flags |= GFastVRamConfig.GBufferVelocity;
// 			GRenderTargetPool.FindFreeElement(RHICmdList, VelocityRTDesc, GBufferVelocity, TEXT("GBufferVelocity"));
// 		}

		// otherwise we have a severe problem
		//check(GBufferA);
	}

	//GBufferRefCount = 1;
}

void FSceneRenderTargets::AllocateScreenShadowMask(ComPtr<PooledRenderTarget>& ScreenShadowMaskTexture)
{
	PooledRenderTargetDesc Desc(PooledRenderTargetDesc::Create2DDesc(GetBufferSizeXY(), PF_B8G8R8A8, FClearValueBinding::White, TexCreate_None, TexCreate_RenderTargetable, false));
	//Desc.Flags |= GFastVRamConfig.ScreenSpaceShadowMask;
	Desc.NumSamples = 1;// GetNumSceneColorMSAASamples(GetCurrentFeatureLevel());
	GRenderTargetPool.FindFreeElement(Desc, ScreenShadowMaskTexture, TEXT("ScreenShadowMaskTexture"));
}

FIntPoint FSceneRenderTargets::GetShadowDepthTextureResolution() const
{
	int32 MaxShadowRes = CurrentMaxShadowResolution;
	const FIntPoint ShadowBufferResolution(
		FMath::Clamp(MaxShadowRes, 1, (int32)GMaxShadowDepthBufferSizeX),
		FMath::Clamp(MaxShadowRes, 1, (int32)GMaxShadowDepthBufferSizeY));

	return ShadowBufferResolution;
}

int32 FSceneRenderTargets::GetCubeShadowDepthZIndex(int32 ShadowResolution) const
{
	static auto CVarMinShadowResolution = 32;// IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.Shadow.MinResolution"));
	FIntPoint ObjectShadowBufferResolution = GetShadowDepthTextureResolution();

	// Use a lower resolution because cubemaps use a lot of memory
	ObjectShadowBufferResolution.X /= 2;
	ObjectShadowBufferResolution.Y /= 2;
	const int32 SurfaceSizes[NumCubeShadowDepthSurfaces] =
	{
		ObjectShadowBufferResolution.X,
		ObjectShadowBufferResolution.X / 2,
		ObjectShadowBufferResolution.X / 4,
		ObjectShadowBufferResolution.X / 8,
		CVarMinShadowResolution
	};

	for (int32 SearchIndex = 0; SearchIndex < NumCubeShadowDepthSurfaces; SearchIndex++)
	{
		if (ShadowResolution >= SurfaceSizes[SearchIndex])
		{
			return SearchIndex;
		}
	}

	assert(0);
	return 0;
}

int32 FSceneRenderTargets::GetCubeShadowDepthZResolution(int32 ShadowIndex) const
{
	assert(ShadowIndex >= 0 && ShadowIndex < NumCubeShadowDepthSurfaces);

	static auto CVarMinShadowResolution = 32;// IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.Shadow.MinResolution"));
	FIntPoint ObjectShadowBufferResolution = GetShadowDepthTextureResolution();

	// Use a lower resolution because cubemaps use a lot of memory
	ObjectShadowBufferResolution.X = FMath::Max(ObjectShadowBufferResolution.X / 2, 1);
	ObjectShadowBufferResolution.Y = FMath::Max(ObjectShadowBufferResolution.Y / 2, 1);
	const int32 SurfaceSizes[NumCubeShadowDepthSurfaces] =
	{
		ObjectShadowBufferResolution.X,
		FMath::Max(ObjectShadowBufferResolution.X / 2, 1),
		FMath::Max(ObjectShadowBufferResolution.X / 4, 1),
		FMath::Max(ObjectShadowBufferResolution.X / 8, 1),
		CVarMinShadowResolution
	};
	return SurfaceSizes[ShadowIndex];
}

const std::shared_ptr<FD3D11Texture2D>& FSceneRenderTargets::GetSceneColorTexture() const
{
	if (!GetSceneColorForCurrentShadingPath())
	{
		static std::shared_ptr<FD3D11Texture2D> NULLTexture;
		return NULLTexture;
		//return GBlackTexture->TextureRHI;
	}
	return SceneColor->ShaderResourceTexture;
}

const std::shared_ptr<FD3D11Texture2D>* FSceneRenderTargets::GetActualDepthTexture() const
{
	const std::shared_ptr<FD3D11Texture2D>* DepthTexture = NULL;
	//if ((CurrentFeatureLevel >= ERHIFeatureLevel::SM4) || IsPCPlatform(GShaderPlatformForFeatureLevel[CurrentFeatureLevel]))
	{
		//if (GSupportsDepthFetchDuringDepthTest)
		{
			DepthTexture = &GetSceneDepthTexture();
		}
		//else
		{
			//DepthTexture = &GetAuxiliarySceneDepthSurface();
		}
	}
	//else if (IsMobilePlatform(GShaderPlatformForFeatureLevel[CurrentFeatureLevel]))
	{
		// TODO: avoid depth texture fetch when shader needs fragment previous depth and device supports framebuffer fetch

		//bool bSceneDepthInAlpha = (GetSceneColor()->GetDesc().Format == PF_FloatRGBA);
		//bool bOnChipDepthFetch = (GSupportsShaderDepthStencilFetch || (bSceneDepthInAlpha && GSupportsShaderFramebufferFetch));
		//
		//if (bOnChipDepthFetch)
		//{
		//	DepthTexture = (const FTexture2DRHIRef*)(&GSystemTextures.DepthDummy->GetRenderTargetItem().ShaderResourceTexture);
		//}
		//else
		{
			//DepthTexture = &GetSceneDepthTexture();
		}
	}

	assert(DepthTexture != NULL);

	return DepthTexture;
}

void FSceneRenderTargets::AllocLightAttenuation()
{

}

FIntPoint FSceneRenderTargets::ComputeDesiredSize(const FSceneViewFamily& ViewFamily)
{
	FIntPoint DesiredFamilyBufferSize = FSceneRenderer::GetDesiredInternalBufferSize(ViewFamily);
	return DesiredFamilyBufferSize;
}

int FSceneRenderTargets::GetGBufferRenderTargets(ERenderTargetLoadAction ColorLoadAction, FD3D11Texture2D* OutRenderTargets[8], int32& OutVelocityRTIndex)
{
	int32 MRTCount = 0;
	OutRenderTargets[MRTCount++] = (FD3D11Texture2D*)GetSceneColorSurface().get();
	OutRenderTargets[MRTCount++] = (FD3D11Texture2D*)GBufferA->TargetableTexture.get();
	OutRenderTargets[MRTCount++] = (FD3D11Texture2D*)GBufferB->TargetableTexture.get();
	OutRenderTargets[MRTCount++] = (FD3D11Texture2D*)GBufferC->TargetableTexture.get();

	return MRTCount;
}

void SetupSceneTextureUniformParameters(FSceneRenderTargets& SceneContext, ESceneTextureSetupMode SetupMode, FSceneTexturesUniformParameters& SceneTextureParameters)
{
	std::shared_ptr<FD3D11Texture2D> WhiteDefault2D = GSystemTextures.WhiteDummy->ShaderResourceTexture;
	std::shared_ptr<FD3D11Texture2D> BlackDefault2D = GSystemTextures.BlackDummy->ShaderResourceTexture;
	std::shared_ptr<FD3D11Texture2D> DepthDefault = GSystemTextures.DepthDummy->ShaderResourceTexture;

	// Scene Color / Depth
	{
		const bool bSetupDepth = (SetupMode & ESceneTextureSetupMode::SceneDepth) != ESceneTextureSetupMode::None;
		ID3D11ShaderResourceView* SceneColorSRV = SceneContext.GetSceneColorTexture() ? SceneContext.GetSceneColorTexture()->GetShaderResourceView() : NULL;
		ID3D11ShaderResourceView* BlackDefault2DSRV = BlackDefault2D->GetShaderResourceView();
		SceneTextureParameters.SceneColorTexture = bSetupDepth ? SceneColorSRV : BlackDefault2DSRV;
		SceneTextureParameters.SceneColorTextureSampler = TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI();
		const std::shared_ptr<FD3D11Texture2D>* ActualDepthTexture = SceneContext.GetActualDepthTexture();
		SceneTextureParameters.SceneDepthTexture = bSetupDepth && ActualDepthTexture ? (*ActualDepthTexture)->GetShaderResourceView() : DepthDefault->GetShaderResourceView();

// 		if (bSetupDepth && SceneContext.IsSeparateTranslucencyPass() && SceneContext.IsDownsampledTranslucencyDepthValid())
// 		{
// 			FIntPoint OutScaledSize;
// 			float OutScale;
// 			SceneContext.GetSeparateTranslucencyDimensions(OutScaledSize, OutScale);
// 
// 			if (OutScale < 1.0f)
// 			{
// 				SceneTextureParameters.SceneDepthTexture = SceneContext.GetDownsampledTranslucencyDepthSurface();
// 			}
// 		}

// 		if (bSetupDepth)
// 		{
// 			SceneTextureParameters.SceneDepthTextureNonMS = GSupportsDepthFetchDuringDepthTest ? SceneContext.GetSceneDepthTexture() : SceneContext.GetAuxiliarySceneDepthSurface();
// 		}
// 		else
// 		{
// 			SceneTextureParameters.SceneDepthTextureNonMS = DepthDefault;
// 		}

		SceneTextureParameters.SceneDepthTextureSampler = TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI();
		//SceneTextureParameters.SceneStencilTexture = bSetupDepth && SceneContext.SceneStencilSRV ? SceneContext.SceneStencilSRV : GNullColorVertexBuffer.VertexBufferSRV;
 	}

	// GBuffer
	{
		const bool bSetupGBuffers = (SetupMode & ESceneTextureSetupMode::GBuffers) != ESceneTextureSetupMode::None;
		//const EShaderPlatform ShaderPlatform = GetFeatureLevelShaderPlatform(FeatureLevel);
		const bool bUseGBuffer = true;//IsUsingGBuffers(ShaderPlatform);
		const bool bCanReadGBufferUniforms = true;//bSetupGBuffers && (bUseGBuffer || IsSimpleForwardShadingEnabled(ShaderPlatform));

		// Allocate the Gbuffer resource uniform buffer.
		const ComPtr<PooledRenderTarget> GBufferAToUse = bCanReadGBufferUniforms && SceneContext.GBufferA ? SceneContext.GBufferA : GSystemTextures.BlackDummy;
		const ComPtr<PooledRenderTarget> GBufferBToUse = bCanReadGBufferUniforms && SceneContext.GBufferB ? SceneContext.GBufferB : GSystemTextures.BlackDummy;
		const ComPtr<PooledRenderTarget> GBufferCToUse = bCanReadGBufferUniforms && SceneContext.GBufferC ? SceneContext.GBufferC : GSystemTextures.BlackDummy;
		//const ComPtr<PooledRenderTarget> GBufferDToUse = bCanReadGBufferUniforms && SceneContext.GBufferD ? SceneContext.GBufferD : GSystemTextures.BlackDummy;
		//const ComPtr<PooledRenderTarget> GBufferEToUse = bCanReadGBufferUniforms && SceneContext.GBufferE ? SceneContext.GBufferE : GSystemTextures.BlackDummy;
		//const ComPtr<PooledRenderTarget> GBufferVelocityToUse = bCanReadGBufferUniforms && SceneContext.GBufferVelocity ? SceneContext.GBufferVelocity : GSystemTextures.BlackDummy;

		SceneTextureParameters.GBufferATexture = GBufferAToUse->ShaderResourceTexture->GetShaderResourceView();
		SceneTextureParameters.GBufferBTexture = GBufferBToUse->ShaderResourceTexture->GetShaderResourceView();
		SceneTextureParameters.GBufferCTexture = GBufferCToUse->ShaderResourceTexture->GetShaderResourceView();
		//SceneTextureParameters.GBufferDTexture = GBufferDToUse->ShaderResourceTexture->GetShaderResourceView();
		//SceneTextureParameters.GBufferETexture = GBufferEToUse->ShaderResourceTexture->GetShaderResourceView();
		//SceneTextureParameters.GBufferVelocityTexture = GBufferVelocityToUse->ShaderResourceTexture->GetShaderResourceView();

		SceneTextureParameters.GBufferATextureNonMS = GBufferAToUse->ShaderResourceTexture->GetShaderResourceView();
		SceneTextureParameters.GBufferBTextureNonMS = GBufferBToUse->ShaderResourceTexture->GetShaderResourceView();
		SceneTextureParameters.GBufferCTextureNonMS = GBufferCToUse->ShaderResourceTexture->GetShaderResourceView();
		//SceneTextureParameters.GBufferDTextureNonMS = GBufferDToUse->ShaderResourceTexture->GetShaderResourceView();
		//SceneTextureParameters.GBufferETextureNonMS = GBufferEToUse->ShaderResourceTexture->GetShaderResourceView();
		//SceneTextureParameters.GBufferVelocityTextureNonMS = GBufferVelocityToUse->ShaderResourceTexture->GetShaderResourceView();

		SceneTextureParameters.GBufferATextureSampler = TStaticSamplerState<>::GetRHI();
		SceneTextureParameters.GBufferBTextureSampler = TStaticSamplerState<>::GetRHI();
		SceneTextureParameters.GBufferCTextureSampler = TStaticSamplerState<>::GetRHI();
		SceneTextureParameters.GBufferDTextureSampler = TStaticSamplerState<>::GetRHI();
		SceneTextureParameters.GBufferETextureSampler = TStaticSamplerState<>::GetRHI();
		SceneTextureParameters.GBufferVelocityTextureSampler = TStaticSamplerState<>::GetRHI();
	}

	// SSAO
	{
		const bool bSetupSSAO = (SetupMode & ESceneTextureSetupMode::SSAO) != ESceneTextureSetupMode::None;
		SceneTextureParameters.ScreenSpaceAOTexture = bSetupSSAO && SceneContext.bScreenSpaceAOIsValid ? SceneContext.ScreenSpaceAO->ShaderResourceTexture->GetShaderResourceView() : WhiteDefault2D->GetShaderResourceView();
		SceneTextureParameters.ScreenSpaceAOTextureSampler = TStaticSamplerState<>::GetRHI();
	}

	// Custom Depth / Stencil
// 	{
// 		const bool bSetupCustomDepth = (SetupMode & ESceneTextureSetupMode::CustomDepth) != ESceneTextureSetupMode::None;
// 
// 		FTextureRHIParamRef CustomDepth = DepthDefault;
// 		FShaderResourceViewRHIParamRef CustomStencilSRV = GNullColorVertexBuffer.VertexBufferSRV;
// 
// 		// if there is no custom depth it's better to have the far distance there
// 		IPooledRenderTarget* CustomDepthTarget = SceneContext.bCustomDepthIsValid ? SceneContext.CustomDepth.GetReference() : 0;
// 		if (bSetupCustomDepth && CustomDepthTarget)
// 		{
// 			CustomDepth = CustomDepthTarget->ShaderResourceTexture;
// 		}
// 
// 		if (bSetupCustomDepth && SceneContext.bCustomDepthIsValid && SceneContext.CustomStencilSRV.GetReference())
// 		{
// 			CustomStencilSRV = SceneContext.CustomStencilSRV;
// 		}
// 
// 		SceneTextureParameters.CustomDepthTexture = CustomDepth;
// 		SceneTextureParameters.CustomDepthTextureSampler = TStaticSamplerState<>::GetRHI();
// 		SceneTextureParameters.CustomDepthTextureNonMS = CustomDepth;
// 		SceneTextureParameters.CustomStencilTexture = CustomStencilSRV;
// 	}

	// Misc
// 	{
// 		// Setup by passes that support it
// 		SceneTextureParameters.EyeAdaptation = GWhiteTexture->TextureRHI;
// 		SceneTextureParameters.SceneColorCopyTexture = BlackDefault2D;
// 		SceneTextureParameters.SceneColorCopyTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
// 	}

}

TUniformBufferPtr<FSceneTexturesUniformParameters> CreateSceneTextureUniformBufferSingleDraw(ESceneTextureSetupMode SceneTextureSetupMode)
{
	FSceneTexturesUniformParameters SceneTextureParameters;
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();
	SetupSceneTextureUniformParameters(SceneContext, SceneTextureSetupMode, SceneTextureParameters);
	return TUniformBufferPtr<FSceneTexturesUniformParameters>::CreateUniformBufferImmediate(SceneTextureParameters);
}


void BindSceneTextureUniformBufferDependentOnShadingPath(const FShader::CompiledShaderInitializerType& Initializer, FShaderUniformBufferParameter& SceneTexturesUniformBuffer /*,FShaderUniformBufferParameter& MobileSceneTexturesUniformBuffer*/)
{
	//if (FSceneInterface::GetShadingPath(FeatureLevel) == EShadingPath::Deferred)
	{
		SceneTexturesUniformBuffer.Bind(Initializer.ParameterMap, FSceneTexturesUniformParameters::GetConstantBufferName().c_str());
		for (auto& Pair : FSceneTexturesUniformParameters::GetSRVs(FSceneTexturesUniformParameters()))
		{
			SceneTexturesUniformBuffer.BindSRV(Initializer.ParameterMap, Pair.first.c_str());
		}
		for (auto& Pair : FSceneTexturesUniformParameters::GetSamplers(FSceneTexturesUniformParameters()))
		{
			SceneTexturesUniformBuffer.BindSampler(Initializer.ParameterMap, Pair.first.c_str());
		}
		//assert(!Initializer.ParameterMap.ContainsParameterAllocation(FMobileSceneTextureUniformParameters::StaticStruct.GetShaderVariableName()), TEXT("Shader for Deferred shading path tried to bind FMobileSceneTextureUniformParameters which is only available in the mobile shading path: %s"), Initializer.Type->GetName());
	}

// 	if (FSceneInterface::GetShadingPath(FeatureLevel) == EShadingPath::Mobile)
// 	{
// 		MobileSceneTexturesUniformBuffer.Bind(Initializer.ParameterMap, FMobileSceneTextureUniformParameters::StaticStruct.GetShaderVariableName());
// 		checkfSlow(!Initializer.ParameterMap.ContainsParameterAllocation(FSceneTexturesUniformParameters::StaticStruct.GetShaderVariableName()), TEXT("Shader for Mobile shading path tried to bind FSceneTexturesUniformParameters which is only available in the deferred shading path: %s"), Initializer.Type->GetName());
// 	}
}
static inline DXGI_FORMAT ConvertTypelessToUnorm(DXGI_FORMAT Format)
{
	// required to prevent 
	// D3D11: ERROR: ID3D11DeviceContext::ResolveSubresource: The Format (0x1b, R8G8B8A8_TYPELESS) is never able to resolve multisampled resources. [ RESOURCE_MANIPULATION ERROR #294: DEVICE_RESOLVESUBRESOURCE_FORMAT_INVALID ]
	// D3D11: **BREAK** enabled for the previous D3D11 message, which was: [ RESOURCE_MANIPULATION ERROR #294: DEVICE_RESOLVESUBRESOURCE_FORMAT_INVALID ]
	switch (Format)
	{
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		return DXGI_FORMAT_R8G8B8A8_UNORM;

	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		return DXGI_FORMAT_B8G8R8A8_UNORM;

	default:
		return Format;
	}
}
void CopyToResolveTarget(FD3D11Texture2D* SourceTextureRHI, FD3D11Texture2D* DestTextureRHI, const FResolveParams& ResolveParams)
{
	if (!SourceTextureRHI || !DestTextureRHI)
	{
		// no need to do anything (sliently ignored)
		return;
	}


	FD3D11Texture2D* SourceTexture2D = SourceTextureRHI;
	FD3D11Texture2D* DestTexture2D = DestTextureRHI;

	// 	FD3D11TextureCube* SourceTextureCube = static_cast<FD3D11TextureCube*>(SourceTextureRHI->GetTextureCube());
	// 	FD3D11TextureCube* DestTextureCube = static_cast<FD3D11TextureCube*>(DestTextureRHI->GetTextureCube());
	// 
	// 	FD3D11Texture3D* SourceTexture3D = static_cast<FD3D11Texture3D*>(SourceTextureRHI->GetTexture3D());
	// 	FD3D11Texture3D* DestTexture3D = static_cast<FD3D11Texture3D*>(DestTextureRHI->GetTexture3D());

	if (SourceTexture2D && DestTexture2D)
	{
		//assert(!SourceTextureCube && !DestTextureCube);
		if (SourceTexture2D != DestTexture2D)
		{
			if (DestTexture2D->GetDepthStencilView(FExclusiveDepthStencil::DepthWrite_StencilWrite)
				&& SourceTextureRHI->IsMultisampled()
				&& !DestTextureRHI->IsMultisampled())
			{
			}
			else 
			{
				DXGI_FORMAT SrcFmt = (DXGI_FORMAT)GPixelFormats[SourceTextureRHI->GetFormat()].PlatformFormat;
				DXGI_FORMAT DstFmt = (DXGI_FORMAT)GPixelFormats[DestTexture2D->GetFormat()].PlatformFormat;

				DXGI_FORMAT Fmt = ConvertTypelessToUnorm((DXGI_FORMAT)GPixelFormats[DestTexture2D->GetFormat()].PlatformFormat);

				// Determine whether a MSAA resolve is needed, or just a copy.
				if (SourceTextureRHI->IsMultisampled() && !DestTexture2D->IsMultisampled())
				{
					D3D11DeviceContext->ResolveSubresource(
						DestTexture2D->GetResource(),
						ResolveParams.DestArrayIndex,
						SourceTexture2D->GetResource(),
						ResolveParams.SourceArrayIndex,
						Fmt
					);
				}
				else
				{
					if (ResolveParams.Rect.IsValid())
					{
						D3D11_BOX SrcBox;

						SrcBox.left = ResolveParams.Rect.X1;
						SrcBox.top = ResolveParams.Rect.Y1;
						SrcBox.front = 0;
						SrcBox.right = ResolveParams.Rect.X2;
						SrcBox.bottom = ResolveParams.Rect.Y2;
						SrcBox.back = 1;

						D3D11DeviceContext->CopySubresourceRegion(DestTexture2D->GetResource(), ResolveParams.DestArrayIndex, ResolveParams.DestRect.X1, ResolveParams.DestRect.Y1, 0, SourceTexture2D->GetResource(), ResolveParams.SourceArrayIndex, &SrcBox);
					}
					else
					{
						D3D11DeviceContext->CopyResource(DestTexture2D->GetResource(), SourceTexture2D->GetResource());
					}
				}
			}
		}
	}
}


FSceneTexturesUniformParameters SceneTexturesUniformParameters;