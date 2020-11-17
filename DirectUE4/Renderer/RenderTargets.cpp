#include "RenderTargets.h"
#include "DeferredShading.h"
#include "SystemTextures.h"
#include "SceneRenderTargetParameters.h"

RenderTargets& RenderTargets::Get()
{
	static RenderTargets Instance;
	return Instance;
}

void RenderTargets::BeginRenderingSceneColor()
{
	AllocSceneColor();
	//D3D11DeviceContext->OMSetRenderTargets(1, &SceneColorRTV, NULL);
}

void RenderTargets::BeginRenderingPrePass(bool bClear)
{
// 	if (bClear)
// 	{
// 		D3D11DeviceContext->OMSetRenderTargets(0, NULL, SceneDepthDSV);
// 		D3D11DeviceContext->ClearDepthStencilView(SceneDepthDSV, D3D11_CLEAR_DEPTH, 0.f, 0);
// 	}
// 	else
// 	{
// 		D3D11DeviceContext->OMSetRenderTargets(0, NULL, SceneDepthDSV);
// 	}
}

void RenderTargets::FinishRenderingPrePass()
{

}

void RenderTargets::BeginRenderingGBuffer(bool bClearColor, const FLinearColor& ClearColor /*= (0, 0, 0, 1)*/)
{
	AllocSceneColor();

// 	ID3D11RenderTargetView* MRT[8];
// 	int32 OutVelocityRTIndex;
// 	int MRTCount =  GetGBufferRenderTargets(MRT, OutVelocityRTIndex);
// 
// 	D3D11DeviceContext->OMSetRenderTargets(MRTCount, MRT, SceneDepthDSV);
// 
// 	D3D11DeviceContext->ClearDepthStencilView(SceneDepthDSV, D3D11_CLEAR_DEPTH, 0.f, 0);
// 
// 	if (bClearColor)
// 	{
// 		for (int i = 0; i < MRTCount; ++i)
// 		{
// 			D3D11DeviceContext->ClearRenderTargetView(MRT[i], (FLOAT*)&ClearColor);
// 		}
// 	}
}

void RenderTargets::FinishRenderingGBuffer()
{

}

void RenderTargets::FinishRendering()
{
	//D3D11DeviceContext->CopyResource(BackBuffer, SceneColorRT);
}

EPixelFormat RenderTargets::GetSceneColorFormat() const
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

void RenderTargets::SetBufferSize(int32 InBufferSizeX, int32 InBufferSizeY)
{
	QuantizeSceneBufferSize(FIntPoint(InBufferSizeX, InBufferSizeY), BufferSize);
}

void RenderTargets::Allocate(const SceneRenderer* Renderer)
{
	FIntPoint DesiredBufferSize = ComputeDesiredSize(Renderer->ViewFamily);

	int GBufferFormat = 1; //CVarGBufferFormat.GetValueOnRenderThread();

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

	CurrentGBufferFormat = GBufferFormat;
	CurrentSceneColorFormat = SceneColorFormat;
	bAllowStaticLighting = bNewAllowStaticLighting;
	//bUseDownsizedOcclusionQueries = bDownsampledOcclusionQueries;
	//CurrentMaxShadowResolution = MaxShadowResolution;
	//CurrentRSMResolution = RSMResolution;
	//CurrentTranslucencyLightingVolumeDim = TranslucencyLightingVolumeDim;
	CurrentMSAACount = MSAACount;
	//CurrentMinShadowResolution = MinShadowResolution;
	//bCurrentLightPropagationVolume = bLightPropagationVolume;

	SetBufferSize(DesiredBufferSize.X,DesiredBufferSize.Y);

	AllocRenderTargets();
}

void RenderTargets::AllocRenderTargets()
{
	AllocateDeferredShadingPathRenderTargets();
}

void RenderTargets::AllocateDeferredShadingPathRenderTargets()
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

void RenderTargets::AllocSceneColor()
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

void RenderTargets::AllocateCommonDepthTargets()
{
	// Create a texture to store the resolved scene depth, and a render-targetable surface to hold the unresolved scene depth.
	PooledRenderTargetDesc Desc(PooledRenderTargetDesc::Create2DDesc(BufferSize, PF_DepthStencil, DefaultDepthClear, TexCreate_None, TexCreate_DepthStencilTargetable, false));
	Desc.NumSamples = 1;// GetNumSceneColorMSAASamples(CurrentFeatureLevel);
	//Desc.Flags |= GFastVRamConfig.SceneDepth;
	GRenderTargetPool.FindFreeElement(Desc, SceneDepthZ, TEXT("SceneDepthZ"));
}

void RenderTargets::GetGBufferADesc(PooledRenderTargetDesc& Desc) const
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

void RenderTargets::AllocGBufferTargets()
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

const FD3D11Texture2D* RenderTargets::GetActualDepthTexture() const
{
	const FD3D11Texture2D* DepthTexture = NULL;
	//if ((CurrentFeatureLevel >= ERHIFeatureLevel::SM4) || IsPCPlatform(GShaderPlatformForFeatureLevel[CurrentFeatureLevel]))
	{
		//if (GSupportsDepthFetchDuringDepthTest)
		{
			DepthTexture = GetSceneDepthTexture();
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

void RenderTargets::AllocLightAttenuation()
{

}

FIntPoint RenderTargets::ComputeDesiredSize(const SceneViewFamily& ViewFamily)
{
	FIntPoint DesiredFamilyBufferSize = SceneRenderer::GetDesiredInternalBufferSize(ViewFamily);
	return DesiredFamilyBufferSize;
}

int RenderTargets::GetGBufferRenderTargets(ERenderTargetLoadAction ColorLoadAction, FD3D11Texture2D* OutRenderTargets[8], int32& OutVelocityRTIndex)
{
	int32 MRTCount = 0;
	OutRenderTargets[MRTCount++] = (FD3D11Texture2D*)GetSceneColorSurface();
	OutRenderTargets[MRTCount++] = (FD3D11Texture2D*)GBufferA->TargetableTexture;
	OutRenderTargets[MRTCount++] = (FD3D11Texture2D*)GBufferB->TargetableTexture;
	OutRenderTargets[MRTCount++] = (FD3D11Texture2D*)GBufferC->TargetableTexture;

	return MRTCount;
}

void SetupSceneTextureUniformParameters(RenderTargets& SceneContext, ESceneTextureSetupMode SetupMode, FSceneTexturesUniformParameters& SceneTextureParameters)
{
	FD3D11Texture2D* WhiteDefault2D = GSystemTextures.WhiteDummy->ShaderResourceTexture;
	FD3D11Texture2D* BlackDefault2D = GSystemTextures.BlackDummy->ShaderResourceTexture;
	FD3D11Texture2D* DepthDefault = GSystemTextures.DepthDummy->ShaderResourceTexture;

	// Scene Color / Depth
	{
		const bool bSetupDepth = (SetupMode & ESceneTextureSetupMode::SceneDepth) != ESceneTextureSetupMode::None;
		SceneTextureParameters.SceneColorTexture = bSetupDepth ? SceneContext.GetSceneColorTexture()->GetShaderResourceView() : BlackDefault2D->GetShaderResourceView();
		SceneTextureParameters.SceneColorTextureSampler = TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI();
		const FD3D11Texture2D* ActualDepthTexture = SceneContext.GetActualDepthTexture();
		SceneTextureParameters.SceneDepthTexture = bSetupDepth && ActualDepthTexture ? ActualDepthTexture->GetShaderResourceView() : DepthDefault->GetShaderResourceView();

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
// 
// 		if (bSetupDepth)
// 		{
// 			SceneTextureParameters.SceneDepthTextureNonMS = GSupportsDepthFetchDuringDepthTest ? SceneContext.GetSceneDepthTexture() : SceneContext.GetAuxiliarySceneDepthSurface();
// 		}
// 		else
// 		{
// 			SceneTextureParameters.SceneDepthTextureNonMS = DepthDefault;
// 		}
// 
// 		SceneTextureParameters.SceneDepthTextureSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
// 		SceneTextureParameters.SceneStencilTexture = bSetupDepth && SceneContext.SceneStencilSRV ? SceneContext.SceneStencilSRV : GNullColorVertexBuffer.VertexBufferSRV;
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
	RenderTargets& SceneContext = RenderTargets::Get();
	SetupSceneTextureUniformParameters(SceneContext, SceneTextureSetupMode, SceneTextureParameters);
	return TUniformBufferPtr<FSceneTexturesUniformParameters>::CreateUniformBufferImmediate(SceneTextureParameters);
}


void BindSceneTextureUniformBufferDependentOnShadingPath(const FShader::CompiledShaderInitializerType& Initializer, FShaderUniformBufferParameter& SceneTexturesUniformBuffer /*,FShaderUniformBufferParameter& MobileSceneTexturesUniformBuffer*/)
{
	//if (FSceneInterface::GetShadingPath(FeatureLevel) == EShadingPath::Deferred)
	{
		SceneTexturesUniformBuffer.Bind(Initializer.ParameterMap, FSceneTexturesUniformParameters::GetConstantBufferName().c_str());
		//assert(!Initializer.ParameterMap.ContainsParameterAllocation(FMobileSceneTextureUniformParameters::StaticStruct.GetShaderVariableName()), TEXT("Shader for Deferred shading path tried to bind FMobileSceneTextureUniformParameters which is only available in the mobile shading path: %s"), Initializer.Type->GetName());
	}

// 	if (FSceneInterface::GetShadingPath(FeatureLevel) == EShadingPath::Mobile)
// 	{
// 		MobileSceneTexturesUniformBuffer.Bind(Initializer.ParameterMap, FMobileSceneTextureUniformParameters::StaticStruct.GetShaderVariableName());
// 		checkfSlow(!Initializer.ParameterMap.ContainsParameterAllocation(FSceneTexturesUniformParameters::StaticStruct.GetShaderVariableName()), TEXT("Shader for Mobile shading path tried to bind FSceneTexturesUniformParameters which is only available in the deferred shading path: %s"), Initializer.Type->GetName());
// 	}
}
