#include "RenderTargets.h"
#include "DeferredShading.h"

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

void RenderTargets::BeginRenderingGBuffer(bool bClearColor, const LinearColor& ClearColor /*= (0, 0, 0, 1)*/)
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
	QuantizeSceneBufferSize(IntPoint(InBufferSizeX, InBufferSizeY), BufferSize);
}

void RenderTargets::Allocate(const SceneRenderer* Renderer)
{
	IntPoint DesiredBufferSize = ComputeDesiredSize(Renderer->ViewFamily);

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
		const LinearColor CurrentClearColor = SceneColorTarget->TargetableTexture->GetClearColor();
		const LinearColor NewClearColor = DefaultColorClear.GetClearColor();
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

void RenderTargets::AllocLightAttenuation()
{

}

IntPoint RenderTargets::ComputeDesiredSize(const SceneViewFamily& ViewFamily)
{
	IntPoint DesiredFamilyBufferSize = SceneRenderer::GetDesiredInternalBufferSize(ViewFamily);
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
