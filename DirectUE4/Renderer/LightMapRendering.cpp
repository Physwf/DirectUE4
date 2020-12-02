#include "LightMapRendering.h"
#include "PrimitiveSceneInfo.h"

const char* GLightmapDefineName[2] =
{
	("LQ_TEXTURE_LIGHTMAP"),
	("HQ_TEXTURE_LIGHTMAP")
};

int32 GNumLightmapCoefficients[2] =
{
	NUM_LQ_LIGHTMAP_COEF,
	NUM_HQ_LIGHTMAP_COEF
};

FPrecomputedLightingParameters PrecomputedLightingParameters;
FEmptyPrecomputedLightingUniformBuffer GEmptyPrecomputedLightingUniformBuffer;

void GetPrecomputedLightingParameters(
	FPrecomputedLightingParameters& Parameters, 
	/*const class FIndirectLightingCache* LightingCache,*/ 
	/*const class FIndirectLightingCacheAllocation* LightingAllocation,*/ 
	FVector VolumetricLightmapLookupPosition,
	uint32 SceneFrameNumber, 
	/*class FVolumetricLightmapSceneData* VolumetricLightmapSceneData,*/ 
	const FLightCacheInterface* LCI)
{
	// FCachedVolumeIndirectLightingPolicy, FCachedPointIndirectLightingPolicy
	{
		/*
		if (VolumetricLightmapSceneData)
		{
			FVolumetricLightmapInterpolation* Interpolation = VolumetricLightmapSceneData->CPUInterpolationCache.Find(VolumetricLightmapLookupPosition);

			if (!Interpolation)
			{
				Interpolation = &VolumetricLightmapSceneData->CPUInterpolationCache.Add(VolumetricLightmapLookupPosition);
				InterpolateVolumetricLightmap(VolumetricLightmapLookupPosition, *VolumetricLightmapSceneData, *Interpolation);
			}

			Interpolation->LastUsedSceneFrameNumber = SceneFrameNumber;

			Parameters.PointSkyBentNormal = Interpolation->PointSkyBentNormal;
			Parameters.DirectionalLightShadowing = Interpolation->DirectionalLightShadowing;

			for (int32 i = 0; i < 3; i++)
			{
				Parameters.IndirectLightingSHCoefficients0[i] = Interpolation->IndirectLightingSHCoefficients0[i];
				Parameters.IndirectLightingSHCoefficients1[i] = Interpolation->IndirectLightingSHCoefficients1[i];
			}

			Parameters.IndirectLightingSHCoefficients2 = Interpolation->IndirectLightingSHCoefficients2;
			Parameters.IndirectLightingSHSingleCoefficient = Interpolation->IndirectLightingSHSingleCoefficient;

			// Unused
			Parameters.IndirectLightingCachePrimitiveAdd = FVector(0, 0, 0);
			Parameters.IndirectLightingCachePrimitiveScale = FVector(1, 1, 1);
			Parameters.IndirectLightingCacheMinUV = FVector(0, 0, 0);
			Parameters.IndirectLightingCacheMaxUV = FVector(1, 1, 1);
		}
		else if (LightingAllocation)
		{
			Parameters.IndirectLightingCachePrimitiveAdd = LightingAllocation->Add;
			Parameters.IndirectLightingCachePrimitiveScale = LightingAllocation->Scale;
			Parameters.IndirectLightingCacheMinUV = LightingAllocation->MinUV;
			Parameters.IndirectLightingCacheMaxUV = LightingAllocation->MaxUV;
			Parameters.PointSkyBentNormal = LightingAllocation->CurrentSkyBentNormal;
			Parameters.DirectionalLightShadowing = LightingAllocation->CurrentDirectionalShadowing;

			for (uint32 i = 0; i < 3; ++i) // RGB
			{
				Parameters.IndirectLightingSHCoefficients0[i] = LightingAllocation->SingleSamplePacked0[i];
				Parameters.IndirectLightingSHCoefficients1[i] = LightingAllocation->SingleSamplePacked1[i];
			}
			Parameters.IndirectLightingSHCoefficients2 = LightingAllocation->SingleSamplePacked2;
			Parameters.IndirectLightingSHSingleCoefficient = Vector4(LightingAllocation->SingleSamplePacked0[0].X, LightingAllocation->SingleSamplePacked0[1].X, LightingAllocation->SingleSamplePacked0[2].X)
				* FSHVector2::ConstantBasisIntegral * .5f; //@todo - why is .5f needed to match directional?
		}
		else
		*/
		{
			Parameters.Constants.IndirectLightingCachePrimitiveAdd = FVector(0, 0, 0);
			Parameters.Constants.IndirectLightingCachePrimitiveScale = FVector(1, 1, 1);
			Parameters.Constants.IndirectLightingCacheMinUV = FVector(0, 0, 0);
			Parameters.Constants.IndirectLightingCacheMaxUV = FVector(1, 1, 1);
			Parameters.Constants.PointSkyBentNormal = Vector4(0, 0, 1, 1);
			Parameters.Constants.DirectionalLightShadowing = 1;

			for (uint32 i = 0; i < 3; ++i) // RGB
			{
				Parameters.Constants.IndirectLightingSHCoefficients0[i] = Vector4(0, 0, 0, 0);
				Parameters.Constants.IndirectLightingSHCoefficients1[i] = Vector4(0, 0, 0, 0);
			}
			Parameters.Constants.IndirectLightingSHCoefficients2 = Vector4(0, 0, 0, 0);
			Parameters.Constants.IndirectLightingSHSingleCoefficient = Vector4(0, 0, 0, 0);
		}

		// If we are using FCachedVolumeIndirectLightingPolicy then InitViews should have updated the lighting cache which would have initialized it
		// However the conditions for updating the lighting cache are complex and fail very occasionally in non-reproducible ways
		// Silently skipping setting the cache texture under failure for now
// 		if (/*FeatureLevel >= ERHIFeatureLevel::SM4*/true && LightingCache && LightingCache->IsInitialized() && GSupportsVolumeTextureRendering)
// 		{
// 			Parameters.IndirectLightingCacheTexture0 = const_cast<FIndirectLightingCache*>(LightingCache)->GetTexture0().ShaderResourceTexture;
// 			Parameters.IndirectLightingCacheTexture1 = const_cast<FIndirectLightingCache*>(LightingCache)->GetTexture1().ShaderResourceTexture;
// 			Parameters.IndirectLightingCacheTexture2 = const_cast<FIndirectLightingCache*>(LightingCache)->GetTexture2().ShaderResourceTexture;
// 
// 			Parameters.IndirectLightingCacheTextureSampler0 = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
// 			Parameters.IndirectLightingCacheTextureSampler1 = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
// 			Parameters.IndirectLightingCacheTextureSampler2 = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
// 		}
// 		else
		//if (FeatureLevel >= ERHIFeatureLevel::ES3_1)
		{
			Parameters.IndirectLightingCacheTexture0 = GBlackVolumeTextureSRV;//  GBlackVolumeTexture->TextureRHI;
			Parameters.IndirectLightingCacheTexture1 = GBlackVolumeTextureSRV;//GBlackVolumeTexture->TextureRHI;
			Parameters.IndirectLightingCacheTexture2 = GBlackVolumeTextureSRV;//GBlackVolumeTexture->TextureRHI;

			Parameters.IndirectLightingCacheTextureSampler0 = GBlackVolumeTextureSamplerState;// GBlackVolumeTexture->SamplerStateRHI;
			Parameters.IndirectLightingCacheTextureSampler1 = GBlackVolumeTextureSamplerState;//GBlackVolumeTexture->SamplerStateRHI;
			Parameters.IndirectLightingCacheTextureSampler2 = GBlackVolumeTextureSamplerState;//GBlackVolumeTexture->SamplerStateRHI;
		}
// 		else
// 		{
// 			Parameters.IndirectLightingCacheTexture0 = GBlackTextureSRV; //GBlackTexture->TextureRHI;
// 			Parameters.IndirectLightingCacheTexture1 = GBlackTextureSRV; //GBlackTexture->TextureRHI;
// 			Parameters.IndirectLightingCacheTexture2 = GBlackTextureSRV; //GBlackTexture->TextureRHI;
// 
// 			Parameters.IndirectLightingCacheTextureSampler0 = GBlackTextureSRV; //GBlackTexture->SamplerStateRHI;
// 			Parameters.IndirectLightingCacheTextureSampler1 = GBlackTextureSRV; //GBlackTexture->SamplerStateRHI;
// 			Parameters.IndirectLightingCacheTextureSampler2 = GBlackTextureSRV; //GBlackTexture->SamplerStateRHI;
// 		}
	}

	// TDistanceFieldShadowsAndLightMapPolicy
	const FShadowMapInteraction ShadowMapInteraction = LCI ? LCI->GetShadowMapInteraction() : FShadowMapInteraction();
	if (ShadowMapInteraction.GetType() == SMIT_Texture)
	{
		ID3D11ShaderResourceView* const ShadowMapTexture = ShadowMapInteraction.GetTexture();
		Parameters.Constants.ShadowMapCoordinateScaleBias = Vector4(ShadowMapInteraction.GetCoordinateScale(), ShadowMapInteraction.GetCoordinateBias());
		Parameters.Constants.StaticShadowMapMasks = Vector4(ShadowMapInteraction.GetChannelValid(0), ShadowMapInteraction.GetChannelValid(1), ShadowMapInteraction.GetChannelValid(2), ShadowMapInteraction.GetChannelValid(3));
		Parameters.Constants.InvUniformPenumbraSizes = ShadowMapInteraction.GetInvUniformPenumbraSize();
		Parameters.StaticShadowTexture = ShadowMapTexture ? ShadowMapTexture/* ShadowMapTexture->TextureReference.TextureReferenceRHI.GetReference()*/ : GWhiteTextureSRV;// GWhiteTexture->TextureRHI;
		Parameters.StaticShadowTextureSampler = (ShadowMapTexture /*&& ShadowMapTexture->Resource*/) ? TStaticSamplerState<>::GetRHI() /*ShadowMapTexture->Resource->SamplerStateRHI*/ : GWhiteTextureSamplerState;// GWhiteTexture->SamplerStateRHI;
	}
	else
	{
		Parameters.Constants.StaticShadowMapMasks = Vector4(1, 1, 1, 1);
		Parameters.Constants.InvUniformPenumbraSizes = Vector4(0, 0, 0, 0);
		Parameters.StaticShadowTexture = GWhiteTextureSRV; //GWhiteTexture->TextureRHI;
		Parameters.StaticShadowTextureSampler = GWhiteTextureSamplerState; //GWhiteTexture->SamplerStateRHI;
	}

	// TLightMapPolicy
	const FLightMapInteraction LightMapInteraction = LCI ? LCI->GetLightMapInteraction() : FLightMapInteraction();
	if (LightMapInteraction.GetType() == LMIT_Texture)
	{
		const bool bAllowHighQualityLightMaps = /*AllowHighQualityLightmaps(FeatureLevel)*/true && LightMapInteraction.AllowsHighQualityLightmaps();

		// Vertex Shader
		const Vector2 LightmapCoordinateScale = LightMapInteraction.GetCoordinateScale();
		const Vector2 LightmapCoordinateBias = LightMapInteraction.GetCoordinateBias();
		Parameters.Constants.LightMapCoordinateScaleBias = Vector4(LightmapCoordinateScale.X, LightmapCoordinateScale.Y, LightmapCoordinateBias.X, LightmapCoordinateBias.Y);

		// Pixel Shader
		ID3D11ShaderResourceView* const LightMapTexture = LightMapInteraction.GetTexture(bAllowHighQualityLightMaps);
		ID3D11ShaderResourceView* const SkyOcclusionTexture = LightMapInteraction.GetSkyOcclusionTexture();
		ID3D11ShaderResourceView* const AOMaterialMaskTexture = LightMapInteraction.GetAOMaterialMaskTexture();

		Parameters.LightMapTexture = LightMapTexture ? LightMapTexture /* LightMapTexture->TextureReference.TextureReferenceRHI.GetReference()*/ : GBlackTextureSRV;// GBlackTexture->TextureRHI;
		Parameters.SkyOcclusionTexture = SkyOcclusionTexture ? SkyOcclusionTexture/*SkyOcclusionTexture->TextureReference.TextureReferenceRHI.GetReference()*/ : GWhiteTextureSRV;// GWhiteTexture->TextureRHI;
		Parameters.AOMaterialMaskTexture = AOMaterialMaskTexture ? AOMaterialMaskTexture /*AOMaterialMaskTexture->TextureReference.TextureReferenceRHI.GetReference()*/ : GBlackTextureSRV;// GBlackTexture->TextureRHI;

		Parameters.LightMapSampler = (LightMapTexture /*&& LightMapTexture->Resource*/) ? TStaticSamplerState<>::GetRHI()/*LightMapTexture->Resource->SamplerStateRHI*/ : GBlackTextureSamplerState;// GBlackTexture->SamplerStateRHI;
		Parameters.SkyOcclusionSampler = (SkyOcclusionTexture /*&& SkyOcclusionTexture->Resource*/) ? TStaticSamplerState<>::GetRHI()/*SkyOcclusionTexture->Resource->SamplerStateRHI*/ : GWhiteTextureSamplerState;// GWhiteTexture->SamplerStateRHI;
		Parameters.AOMaterialMaskSampler = (AOMaterialMaskTexture /*&& AOMaterialMaskTexture->Resource*/) ? TStaticSamplerState<>::GetRHI()/*AOMaterialMaskTexture->Resource->SamplerStateRHI*/ : GBlackTextureSamplerState;// GBlackTexture->SamplerStateRHI;

		const uint32 NumCoef = bAllowHighQualityLightMaps ? NUM_HQ_LIGHTMAP_COEF : NUM_LQ_LIGHTMAP_COEF;
		const Vector4* Scales = LightMapInteraction.GetScaleArray();
		const Vector4* Adds = LightMapInteraction.GetAddArray();
		for (uint32 CoefIndex = 0; CoefIndex < NumCoef; ++CoefIndex)
		{
			Parameters.Constants.LightMapScale[CoefIndex] = Scales[CoefIndex];
			Parameters.Constants.LightMapAdd[CoefIndex] = Adds[CoefIndex];
		}
	}
	else
	{
		// Vertex Shader
		Parameters.Constants.LightMapCoordinateScaleBias = Vector4(1, 1, 0, 0);

		// Pixel Shader
		Parameters.LightMapTexture = GBlackTextureSRV;// GBlackTexture->TextureRHI;
		Parameters.SkyOcclusionTexture = GWhiteTextureSRV;// GWhiteTexture->TextureRHI;
		Parameters.AOMaterialMaskTexture = GBlackTextureSRV;// GBlackTexture->TextureRHI;

		Parameters.LightMapSampler = GBlackTextureSamplerState;// GBlackTexture->SamplerStateRHI;
		Parameters.SkyOcclusionSampler = GWhiteTextureSamplerState;// GWhiteTexture->SamplerStateRHI;
		Parameters.AOMaterialMaskSampler = GBlackTextureSamplerState;// GBlackTexture->SamplerStateRHI;

		const uint32 NumCoef = FMath::Max<uint32>(NUM_HQ_LIGHTMAP_COEF, NUM_LQ_LIGHTMAP_COEF);
		for (uint32 CoefIndex = 0; CoefIndex < NumCoef; ++CoefIndex)
		{
			Parameters.Constants.LightMapScale[CoefIndex] = Vector4(1, 1, 1, 1);
			Parameters.Constants.LightMapAdd[CoefIndex] = Vector4(0, 0, 0, 0);
		}
	}
}

std::shared_ptr<FUniformBuffer> CreatePrecomputedLightingUniformBuffer(
	/*EUniformBufferUsage BufferUsage,*/ 
	/*const class FIndirectLightingCache* LightingCache,*/ 
	/*const class FIndirectLightingCacheAllocation* LightingAllocation,*/ 
	FVector VolumetricLightmapLookupPosition, 
	uint32 SceneFrameNumber, 
	/*FVolumetricLightmapSceneData* VolumetricLightmapSceneData,*/ 
	const FLightCacheInterface* LCI)
{
	FPrecomputedLightingParameters Parameters;
	GetPrecomputedLightingParameters(Parameters, /*LightingCache, LightingAllocation,*/ VolumetricLightmapLookupPosition, SceneFrameNumber, /*VolumetricLightmapSceneData,*/ LCI);
	//return FPrecomputedLightingParameters::CreateUniformBuffer(Parameters, BufferUsage);
	return TUniformBufferPtr<FPrecomputedLightingParameters>::CreateUniformBufferImmediate(Parameters);
}

void FCachedPointIndirectLightingPolicy::ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
{
	OutEnvironment.SetDefine(("CACHED_POINT_INDIRECT_LIGHTING"), ("1"));
}

void FUniformLightMapPolicy::SetMesh(
	const FSceneView& View, 
	const FPrimitiveSceneProxy* PrimitiveSceneProxy, 
	const VertexParametersType* VertexShaderParameters, 
	const PixelParametersType* PixelShaderParameters, 
	FShader* VertexShader, 
	FShader* PixelShader, 
	const FVertexFactory* VertexFactory, 
	const FMaterialRenderProxy* MaterialRenderProxy,
	const FLightCacheInterface* LCI) const
{
	FUniformBuffer* PrecomputedLightingBuffer = NULL;

	// The buffer is not cached to prevent updating the static mesh draw lists when it changes (for instance when streaming new mips)
	if (LCI)
	{
		PrecomputedLightingBuffer = LCI->GetPrecomputedLightingBuffer();
	}
	if (!PrecomputedLightingBuffer && PrimitiveSceneProxy && PrimitiveSceneProxy->GetPrimitiveSceneInfo())
	{
		PrecomputedLightingBuffer = PrimitiveSceneProxy->GetPrimitiveSceneInfo()->IndirectLightingCacheUniformBuffer.get();
	}
	if (!PrecomputedLightingBuffer)
	{
		PrecomputedLightingBuffer = GEmptyPrecomputedLightingUniformBuffer.GetUniformBufferRHI().get();
	}

	if (VertexShaderParameters && VertexShaderParameters->BufferParameter.IsBound())
	{
		SetUniformBufferParameter(VertexShader->GetVertexShader(), VertexShaderParameters->BufferParameter, PrecomputedLightingBuffer);
	}
	if (PixelShaderParameters && PixelShaderParameters->BufferParameter.IsBound())
	{
		SetUniformBufferParameter(PixelShader->GetPixelShader(), PixelShaderParameters->BufferParameter, PrecomputedLightingBuffer);
	}
}
