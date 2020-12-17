#include "DeferredShading.h"
#include "Scene.h"
#include "DepthOnlyRendering.h"
#include "SceneOcclusion.h"
#include "BasePassRendering.h"
#include "ShadowRendering.h"
#include "LightRendering.h"
#include "AtmosphereRendering.h"
#include "RenderTargets.h"
#include "SceneFilterRendering.h"
#include "CompositionLighting.h"
#include "SystemTextures.h"
#include "ShaderCompiler.h"
#include "GlobalShader.h"
#include "PrimitiveSceneInfo.h"
#include "GPUProfiler.h"

#include <new>

uint32 GFrameNumberRenderThread;
uint32 GFrameNumber = 1;

void InitShading()
{
	GFrameNumberRenderThread = 1;
	GSystemTextures.InitializeTextures();
	InitializeShaderTypes();
	InitConstantBuffers();
	CompileGlobalShaderMap(false);

	InitScreenRectangleResources();

	extern void InitStencilingGeometryResources();
	InitStencilingGeometryResources();
}
FViewInfo::FViewInfo(const ViewInitOptions& InitOptions)
	: FSceneView(InitOptions),
	CachedViewUniformShaderParameters(NULL)
{
	Init();
}

FViewInfo::FViewInfo(const FSceneView* InView)
	: FSceneView(*InView),
	CachedViewUniformShaderParameters(NULL)
{
	Init();
}

FViewInfo::~FViewInfo()
{
	if (CachedViewUniformShaderParameters)
		delete CachedViewUniformShaderParameters;
}

FIntPoint FViewInfo::GetSecondaryViewRectSize() const
{
	return FIntPoint(
		FMath::CeilToInt(UnscaledViewRect.Width() * Family->SecondaryViewFraction),
		FMath::CeilToInt(UnscaledViewRect.Height() * Family->SecondaryViewFraction));
}

void UpdateNoiseTextureParameters(FViewUniformShaderParameters& ViewUniformShaderParameters)
{

};

void SetupPrecomputedVolumetricLightmapUniformBufferParameters(const FScene* Scene, FViewUniformShaderParameters& ViewUniformShaderParameters)
{

};

void FViewInfo::SetupUniformBufferParameters(
	FSceneRenderTargets& SceneContext,
	const FViewMatrices& InViewMatrices,
	const FViewMatrices& InPrevViewMatrices,
	FBox* OutTranslucentCascadeBoundsArray,
	int32 NumTranslucentCascades,
	FViewUniformShaderParameters& ViewUniformParameters) const
{
	//check(Family);

	// Create the view's uniform buffer.
	bool bIsMobileMultiViewEnabled = false;
	// Mobile multi-view is not side by side
	const FIntRect EffectiveViewRect = (bIsMobileMultiViewEnabled) ? FIntRect(0, 0, ViewRect.Width(), ViewRect.Height()) : ViewRect;

	// TODO: We should use a view and previous view uniform buffer to avoid code duplication and keep consistency
	SetupCommonViewUniformBufferParameters(
		ViewUniformParameters,
		SceneContext.GetBufferSizeXY(),
		SceneContext.GetMSAACount(),
		EffectiveViewRect,
		InViewMatrices,
		InPrevViewMatrices
	);

	//const bool bCheckerboardSubsurfaceRendering = FRCPassPostProcessSubsurface::RequiresCheckerboardSubsurfaceRendering(SceneContext.GetSceneColorFormat());
	//ViewUniformParameters.bCheckerboardSubsurfaceProfileRendering = bCheckerboardSubsurfaceRendering ? 1.0f : 0.0f;

	FScene* Scene = nullptr;

	if (Family->Scene)
	{
		Scene = Family->Scene;
	}

	if (Scene)
	{
		// 		if (Scene->SimpleDirectionalLight)
		// 		{
		// 			ViewUniformParameters.DirectionalLightColor = Scene->SimpleDirectionalLight->Proxy->GetColor() / PI;
		// 			ViewUniformParameters.DirectionalLightDirection = -Scene->SimpleDirectionalLight->Proxy->GetDirection();
		// 		}
		// 		else
		// 		{
		// 			ViewUniformParameters.DirectionalLightColor = FLinearColor::Black;
		// 			ViewUniformParameters.DirectionalLightDirection = FVector::ZeroVector;
		// 		}

		// Atmospheric fog parameters
		if (/*ShouldRenderAtmosphere(*Family) &&*/ Scene->AtmosphericFog)
		{
			ViewUniformParameters.Constants.AtmosphericFogSunPower = Scene->AtmosphericFog->SunMultiplier;
			ViewUniformParameters.Constants.AtmosphericFogPower = Scene->AtmosphericFog->FogMultiplier;
			ViewUniformParameters.Constants.AtmosphericFogDensityScale = Scene->AtmosphericFog->InvDensityMultiplier;
			ViewUniformParameters.Constants.AtmosphericFogDensityOffset = Scene->AtmosphericFog->DensityOffset;
			ViewUniformParameters.Constants.AtmosphericFogGroundOffset = Scene->AtmosphericFog->GroundOffset;
			ViewUniformParameters.Constants.AtmosphericFogDistanceScale = Scene->AtmosphericFog->DistanceScale;
			ViewUniformParameters.Constants.AtmosphericFogAltitudeScale = Scene->AtmosphericFog->AltitudeScale;
			ViewUniformParameters.Constants.AtmosphericFogHeightScaleRayleigh = Scene->AtmosphericFog->RHeight;
			ViewUniformParameters.Constants.AtmosphericFogStartDistance = Scene->AtmosphericFog->StartDistance;
			ViewUniformParameters.Constants.AtmosphericFogDistanceOffset = Scene->AtmosphericFog->DistanceOffset;
			ViewUniformParameters.Constants.AtmosphericFogSunDiscScale = Scene->AtmosphericFog->SunDiscScale;
			ViewUniformParameters.Constants.AtmosphericFogSunColor = Scene->SunLight ? Scene->SunLight->Proxy->GetColor() : Scene->AtmosphericFog->DefaultSunColor;
			ViewUniformParameters.Constants.AtmosphericFogSunDirection = Scene->SunLight ? -Scene->SunLight->Proxy->GetDirection() : -Scene->AtmosphericFog->DefaultSunDirection;
			ViewUniformParameters.Constants.AtmosphericFogRenderMask = Scene->AtmosphericFog->RenderFlag & (EAtmosphereRenderFlag::E_DisableGroundScattering | EAtmosphereRenderFlag::E_DisableSunDisk);
			ViewUniformParameters.Constants.AtmosphericFogInscatterAltitudeSampleNum = Scene->AtmosphericFog->InscatterAltitudeSampleNum;
		}
		else
		{
			ViewUniformParameters.Constants.AtmosphericFogSunPower = 0.f;
			ViewUniformParameters.Constants.AtmosphericFogPower = 0.f;
			ViewUniformParameters.Constants.AtmosphericFogDensityScale = 0.f;
			ViewUniformParameters.Constants.AtmosphericFogDensityOffset = 0.f;
			ViewUniformParameters.Constants.AtmosphericFogGroundOffset = 0.f;
			ViewUniformParameters.Constants.AtmosphericFogDistanceScale = 0.f;
			ViewUniformParameters.Constants.AtmosphericFogAltitudeScale = 0.f;
			ViewUniformParameters.Constants.AtmosphericFogHeightScaleRayleigh = 0.f;
			ViewUniformParameters.Constants.AtmosphericFogStartDistance = 0.f;
			ViewUniformParameters.Constants.AtmosphericFogDistanceOffset = 0.f;
			ViewUniformParameters.Constants.AtmosphericFogSunDiscScale = 1.f;
			//Added check so atmospheric light color and vector can use a directional light without needing an atmospheric fog actor in the scene
			// 			ViewUniformParameters.AtmosphericFogSunColor = GScene->SunLight ? GScene->SunLight->Proxy->GetColor() : FLinearColor::Black;
			// 			ViewUniformParameters.AtmosphericFogSunDirection = GScene->SunLight ? -GScene->SunLight->Proxy->GetDirection() : FVector::ZeroVector;
			// 			ViewUniformParameters.AtmosphericFogRenderMask = EAtmosphereRenderFlag::E_EnableAll;
			// 			ViewUniformParameters.AtmosphericFogInscatterAltitudeSampleNum = 0;
		}
	}
	else
	{
		// Atmospheric fog parameters
		ViewUniformParameters.Constants.AtmosphericFogSunPower = 0.f;
		ViewUniformParameters.Constants.AtmosphericFogPower = 0.f;
		ViewUniformParameters.Constants.AtmosphericFogDensityScale = 0.f;
		ViewUniformParameters.Constants.AtmosphericFogDensityOffset = 0.f;
		ViewUniformParameters.Constants.AtmosphericFogGroundOffset = 0.f;
		ViewUniformParameters.Constants.AtmosphericFogDistanceScale = 0.f;
		ViewUniformParameters.Constants.AtmosphericFogAltitudeScale = 0.f;
		ViewUniformParameters.Constants.AtmosphericFogHeightScaleRayleigh = 0.f;
		ViewUniformParameters.Constants.AtmosphericFogStartDistance = 0.f;
		ViewUniformParameters.Constants.AtmosphericFogDistanceOffset = 0.f;
		ViewUniformParameters.Constants.AtmosphericFogSunDiscScale = 1.f;
		// 		ViewUniformParameters.AtmosphericFogSunColor = LinearColor::Black;
		// 		ViewUniformParameters.AtmosphericFogSunDirection = Vector::ZeroVector;
		// 		ViewUniformParameters.AtmosphericFogRenderMask = EAtmosphereRenderFlag::E_EnableAll;
		// 		ViewUniformParameters.AtmosphericFogInscatterAltitudeSampleNum = 0;
	}

	ViewUniformParameters.AtmosphereTransmittanceTexture = /*OrBlack2DIfNull*/(AtmosphereTransmittanceTexture);
	ViewUniformParameters.AtmosphereIrradianceTexture = /*OrBlack2DIfNull*/(AtmosphereIrradianceTexture);
	ViewUniformParameters.AtmosphereInscatterTexture = /*OrBlack3DIfNull*/(AtmosphereInscatterTexture);

	ViewUniformParameters.AtmosphereTransmittanceTextureSampler = TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT>::GetRHI();
	ViewUniformParameters.AtmosphereIrradianceTextureSampler = TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT>::GetRHI();
	ViewUniformParameters.AtmosphereInscatterTextureSampler = TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT>::GetRHI();

	// This should probably be in SetupCommonViewUniformBufferParameters, but drags in too many dependencies
	UpdateNoiseTextureParameters(ViewUniformParameters);

	//SetupDefaultGlobalDistanceFieldUniformBufferParameters(ViewUniformParameters);

	//SetupVolumetricFogUniformBufferParameters(ViewUniformParameters);

	SetupPrecomputedVolumetricLightmapUniformBufferParameters(Scene, ViewUniformParameters);

	// Setup view's shared sampler for material texture sampling.
	// 	{
	// 		const float GlobalMipBias = UTexture2D::GetGlobalMipMapLODBias();
	// 
	// 		float FinalMaterialTextureMipBias = GlobalMipBias;
	// 
	// 		if (bIsValidWorldTextureGroupSamplerFilter && PrimaryScreenPercentageMethod == EPrimaryScreenPercentageMethod::TemporalUpscale)
	// 		{
	// 			ViewUniformParameters.MaterialTextureMipBias = MaterialTextureMipBias;
	// 			ViewUniformParameters.MaterialTextureDerivativeMultiply = FMath::Pow(2.0f, MaterialTextureMipBias);
	// 
	// 			FinalMaterialTextureMipBias += MaterialTextureMipBias;
	// 		}
	// 
	// 		FSamplerStateRHIRef WrappedSampler = nullptr;
	// 		FSamplerStateRHIRef ClampedSampler = nullptr;
	// 
	// 		if (FMath::Abs(FinalMaterialTextureMipBias - GlobalMipBias) < KINDA_SMALL_NUMBER)
	// 		{
	// 			WrappedSampler = Wrap_WorldGroupSettings->SamplerStateRHI;
	// 			ClampedSampler = Clamp_WorldGroupSettings->SamplerStateRHI;
	// 		}
	// 		else if (ViewState && FMath::Abs(ViewState->MaterialTextureCachedMipBias - FinalMaterialTextureMipBias) < KINDA_SMALL_NUMBER)
	// 		{
	// 			WrappedSampler = ViewState->MaterialTextureBilinearWrapedSamplerCache;
	// 			ClampedSampler = ViewState->MaterialTextureBilinearClampedSamplerCache;
	// 		}
	// 		else
	// 		{
	// 			check(bIsValidWorldTextureGroupSamplerFilter);
	// 
	// 			WrappedSampler = RHICreateSamplerState(FSamplerStateInitializerRHI(WorldTextureGroupSamplerFilter, AM_Wrap, AM_Wrap, AM_Wrap, FinalMaterialTextureMipBias));
	// 			ClampedSampler = RHICreateSamplerState(FSamplerStateInitializerRHI(WorldTextureGroupSamplerFilter, AM_Clamp, AM_Clamp, AM_Clamp, FinalMaterialTextureMipBias));
	// 		}
	// 
	// 		// At this point, a sampler must be set.
	// 		check(WrappedSampler.IsValid());
	// 		check(ClampedSampler.IsValid());
	// 
	// 		ViewUniformParameters.MaterialTextureBilinearWrapedSampler = WrappedSampler;
	// 		ViewUniformParameters.MaterialTextureBilinearClampedSampler = ClampedSampler;
	// 
	// 		// Update view state's cached sampler.
	// 		if (ViewState && ViewState->MaterialTextureBilinearWrapedSamplerCache != WrappedSampler)
	// 		{
	// 			ViewState->MaterialTextureCachedMipBias = FinalMaterialTextureMipBias;
	// 			ViewState->MaterialTextureBilinearWrapedSamplerCache = WrappedSampler;
	// 			ViewState->MaterialTextureBilinearClampedSamplerCache = ClampedSampler;
	// 		}
	// 	}

	uint32 StateFrameIndexMod8 = 0;

	// 	if (State)
	// 	{
	// 		ViewUniformParameters.TemporalAAParams = FVector4(
	// 			ViewState->GetCurrentTemporalAASampleIndex(),
	// 			ViewState->GetCurrentTemporalAASampleCount(),
	// 			TemporalJitterPixels.X,
	// 			TemporalJitterPixels.Y);
	// 
	// 		StateFrameIndexMod8 = ViewState->GetFrameIndexMod8();
	// 	}
	// 	else
	// 	{
	// 		ViewUniformParameters.TemporalAAParams = FVector4(0, 1, 0, 0);
	// 	}
	// 
	// 	ViewUniformParameters.StateFrameIndexMod8 = StateFrameIndexMod8;
	// 
	// 	{
	// 		// If rendering in stereo, the other stereo passes uses the left eye's translucency lighting volume.
	// 		const FViewInfo* PrimaryView = this;
	// 		if (IStereoRendering::IsASecondaryView(StereoPass))
	// 		{
	// 			if (Family->Views.IsValidIndex(0))
	// 			{
	// 				const FSceneView* LeftEyeView = Family->Views[0];
	// 				if (LeftEyeView->bIsViewInfo && LeftEyeView->StereoPass == eSSP_LEFT_EYE)
	// 				{
	// 					PrimaryView = static_cast<const FViewInfo*>(LeftEyeView);
	// 				}
	// 			}
	// 		}
	// 		PrimaryView->CalcTranslucencyLightingVolumeBounds(OutTranslucentCascadeBoundsArray, NumTranslucentCascades);
	// 	}
	// 
	// 	for (int32 CascadeIndex = 0; CascadeIndex < NumTranslucentCascades; CascadeIndex++)
	// 	{
	// 		const float VolumeVoxelSize = (OutTranslucentCascadeBoundsArray[CascadeIndex].Max.X - OutTranslucentCascadeBoundsArray[CascadeIndex].Min.X) / GTranslucencyLightingVolumeDim;
	// 		const FVector VolumeSize = OutTranslucentCascadeBoundsArray[CascadeIndex].Max - OutTranslucentCascadeBoundsArray[CascadeIndex].Min;
	// 		ViewUniformParameters.TranslucencyLightingVolumeMin[CascadeIndex] = FVector4(OutTranslucentCascadeBoundsArray[CascadeIndex].Min, 1.0f / GTranslucencyLightingVolumeDim);
	// 		ViewUniformParameters.TranslucencyLightingVolumeInvSize[CascadeIndex] = FVector4(FVector(1.0f) / VolumeSize, VolumeVoxelSize);
	// 	}
	// 
	// 	ViewUniformParameters.PreExposure = PreExposure;
	// 	ViewUniformParameters.OneOverPreExposure = 1.f / PreExposure;
	// 
	// 	ViewUniformParameters.DepthOfFieldFocalDistance = FinalPostProcessSettings.DepthOfFieldFocalDistance;
	// 	ViewUniformParameters.DepthOfFieldSensorWidth = FinalPostProcessSettings.DepthOfFieldSensorWidth;
	// 	ViewUniformParameters.DepthOfFieldFocalRegion = FinalPostProcessSettings.DepthOfFieldFocalRegion;
	// 	// clamped to avoid div by 0 in shader
	// 	ViewUniformParameters.DepthOfFieldNearTransitionRegion = FMath::Max(0.01f, FinalPostProcessSettings.DepthOfFieldNearTransitionRegion);
	// 	// clamped to avoid div by 0 in shader
	// 	ViewUniformParameters.DepthOfFieldFarTransitionRegion = FMath::Max(0.01f, FinalPostProcessSettings.DepthOfFieldFarTransitionRegion);
	// 	ViewUniformParameters.DepthOfFieldScale = FinalPostProcessSettings.DepthOfFieldScale;
	// 	ViewUniformParameters.DepthOfFieldFocalLength = 50.0f;
	// 
	// 	ViewUniformParameters.bSubsurfacePostprocessEnabled = GCompositionLighting.IsSubsurfacePostprocessRequired() ? 1.0f : 0.0f;
	// 
	// 	{
	// 		// This is the CVar default
	// 		float Value = 1.0f;
	// 
	// 		// Compiled out in SHIPPING to make cheating a bit harder.
	// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// 		Value = CVarGeneralPurposeTweak.GetValueOnRenderThread();
	// #endif
	// 
	// 		ViewUniformParameters.GeneralPurposeTweak = Value;
	// 	}
	// 
	// 	ViewUniformParameters.DemosaicVposOffset = 0.0f;
	// 	{
	// 		ViewUniformParameters.DemosaicVposOffset = CVarDemosaicVposOffset.GetValueOnRenderThread();
	// 	}
	// 
	// 	ViewUniformParameters.IndirectLightingColorScale = FVector(FinalPostProcessSettings.IndirectLightingColor.R * FinalPostProcessSettings.IndirectLightingIntensity,
	// 		FinalPostProcessSettings.IndirectLightingColor.G * FinalPostProcessSettings.IndirectLightingIntensity,
	// 		FinalPostProcessSettings.IndirectLightingColor.B * FinalPostProcessSettings.IndirectLightingIntensity);
	// 
	// 	ViewUniformParameters.NormalCurvatureToRoughnessScaleBias.X = FMath::Clamp(CVarNormalCurvatureToRoughnessScale.GetValueOnAnyThread(), 0.0f, 2.0f);
	// 	ViewUniformParameters.NormalCurvatureToRoughnessScaleBias.Y = FMath::Clamp(CVarNormalCurvatureToRoughnessBias.GetValueOnAnyThread(), -1.0f, 1.0f);
	// 	ViewUniformParameters.NormalCurvatureToRoughnessScaleBias.Z = FMath::Clamp(CVarNormalCurvatureToRoughnessExponent.GetValueOnAnyThread(), .05f, 20.0f);
	// 
	// 	ViewUniformParameters.RenderingReflectionCaptureMask = bIsReflectionCapture ? 1.0f : 0.0f;
	// 
	// 	ViewUniformParameters.AmbientCubemapTint = FinalPostProcessSettings.AmbientCubemapTint;
	// 	ViewUniformParameters.AmbientCubemapIntensity = FinalPostProcessSettings.AmbientCubemapIntensity;
	// 
	// 	{
	// 		// Enables HDR encoding mode selection without recompile of all PC shaders during ES2 emulation.
	// 		ViewUniformParameters.HDR32bppEncodingMode = 0.0f;
	// 		if (IsMobileHDR32bpp())
	// 		{
	// 			EMobileHDRMode MobileHDRMode = GetMobileHDRMode();
	// 			switch (MobileHDRMode)
	// 			{
	// 			case EMobileHDRMode::EnabledMosaic:
	// 				ViewUniformParameters.HDR32bppEncodingMode = 1.0f;
	// 				break;
	// 			case EMobileHDRMode::EnabledRGBE:
	// 				ViewUniformParameters.HDR32bppEncodingMode = 2.0f;
	// 				break;
	// 			case EMobileHDRMode::EnabledRGBA8:
	// 				ViewUniformParameters.HDR32bppEncodingMode = 3.0f;
	// 				break;
	// 			default:
	// 				checkNoEntry();
	// 				break;
	// 			}
	// 		}
	// 	}
	// 
	// 	ViewUniformParameters.CircleDOFParams = CircleDofHalfCoc(*this);
	// 
	// 	if (Family->Scene)
	// 	{
	// 		Scene = Family->Scene->GetRenderScene();
	// 	}
	// 
	// 

	// 

	// 	ERHIFeatureLevel::Type RHIFeatureLevel = Scene == nullptr ? GMaxRHIFeatureLevel : Scene->GetFeatureLevel();

	if (Scene && Scene->SkyLight)
	{
		FSkyLightSceneProxy* SkyLight = Scene->SkyLight;

		ViewUniformParameters.Constants.SkyLightColor = SkyLight->LightColor;

		bool bApplyPrecomputedBentNormalShadowing = SkyLight->bCastShadows && SkyLight->bWantsStaticShadowing;

		ViewUniformParameters.Constants.SkyLightParameters = bApplyPrecomputedBentNormalShadowing ? 1.f : 0;
		ViewUniformParameters.Constants.SkyLightColor = { 1.0f,1.0f, 1.0f };
		ViewUniformParameters.Constants.SkyLightParameters = 1.f;
	}
	else
	{
		ViewUniformParameters.Constants.SkyLightColor = FLinearColor::Black;
		ViewUniformParameters.Constants.SkyLightParameters = 0;
	}

	// Make sure there's no padding since we're going to cast to FVector4*
	//checkSlow(sizeof(ViewUniformParameters.SkyIrradianceEnvironmentMap) == sizeof(FVector4) * 7);
	SetupSkyIrradianceEnvironmentMapConstants((Vector4*)&ViewUniformParameters.Constants.SkyIrradianceEnvironmentMap);

	// 	ViewUniformParameters.MobilePreviewMode =
	// 		(GIsEditor &&
	// 		(RHIFeatureLevel == ERHIFeatureLevel::ES2 || RHIFeatureLevel == ERHIFeatureLevel::ES3_1) &&
	// 			GMaxRHIFeatureLevel > ERHIFeatureLevel::ES3_1) ? 1.0f : 0.0f;

	// Padding between the left and right eye may be introduced by an HMD, which instanced stereo needs to account for.
	// 	if ((Family != nullptr) && (StereoPass != eSSP_FULL) && (Family->Views.Num() > 1))
	// 	{
	// 		check(Family->Views.Num() >= 2);
	// 
	// 		// The static_cast<const FViewInfo*> is fine because when executing this method, we know that
	// 		// Family::Views point to multiple FViewInfo, since of them is <this>.
	// 		const float StereoViewportWidth = float(
	// 			static_cast<const FViewInfo*>(Family->Views[1])->ViewRect.Max.X -
	// 			static_cast<const FViewInfo*>(Family->Views[0])->ViewRect.Min.X);
	// 		const float EyePaddingSize = float(
	// 			static_cast<const FViewInfo*>(Family->Views[1])->ViewRect.Min.X -
	// 			static_cast<const FViewInfo*>(Family->Views[0])->ViewRect.Max.X);
	// 
	// 		ViewUniformParameters.HMDEyePaddingOffset = (StereoViewportWidth - EyePaddingSize) / StereoViewportWidth;
	// 	}
	// 	else
	// 	{
	// 		ViewUniformParameters.HMDEyePaddingOffset = 1.0f;
	// 	}
	// 
	// 	ViewUniformParameters.ReflectionCubemapMaxMip = FMath::FloorLog2(UReflectionCaptureComponent::GetReflectionCaptureSize());
	// 
	// 	ViewUniformParameters.ShowDecalsMask = Family->EngineShowFlags.Decals ? 1.0f : 0.0f;
	// 
	// 	extern int32 GDistanceFieldAOSpecularOcclusionMode;
	// 	ViewUniformParameters.DistanceFieldAOSpecularOcclusionMode = GDistanceFieldAOSpecularOcclusionMode;
	// 
	// 	ViewUniformParameters.IndirectCapsuleSelfShadowingIntensity = Scene ? Scene->DynamicIndirectShadowsSelfShadowingIntensity : 1.0f;
	// 
	// 	extern FVector GetReflectionEnvironmentRoughnessMixingScaleBiasAndLargestWeight();
	// 	ViewUniformParameters.ReflectionEnvironmentRoughnessMixingScaleBiasAndLargestWeight = GetReflectionEnvironmentRoughnessMixingScaleBiasAndLargestWeight();
	// 
	// 	ViewUniformParameters.StereoPassIndex = (StereoPass <= eSSP_LEFT_EYE) ? 0 : (StereoPass == eSSP_RIGHT_EYE) ? 1 : StereoPass - eSSP_MONOSCOPIC_EYE + 1;
	// 	ViewUniformParameters.StereoIPD = StereoIPD;
	// 
	// 	ViewUniformParameters.PreIntegratedBRDF = GEngine->PreIntegratedSkinBRDFTexture->Resource->TextureRHI;

}


void FViewInfo::InitRHIResources()
{
	FBox VolumeBounds[TVC_MAX];

	CachedViewUniformShaderParameters = new FViewUniformShaderParameters();

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

	SetupUniformBufferParameters(
		SceneContext,
		VolumeBounds,
		TVC_MAX,
		*CachedViewUniformShaderParameters);
	ViewUniformBuffer = TUniformBufferPtr<FViewUniformShaderParameters>::CreateUniformBufferImmediate(*CachedViewUniformShaderParameters); //TUniformBufferRef<FViewUniformShaderParameters>::CreateUniformBufferImmediate(*CachedViewUniformShaderParameters, UniformBuffer_SingleFrame);
}
// These are not real view infos, just dumb memory blocks
static std::vector<FViewInfo*> ViewInfoSnapshots;
// these are never freed, even at program shutdown
static std::vector<FViewInfo*> FreeViewInfoSnapshots;

FViewInfo* FViewInfo::CreateSnapshot() const
{
	FViewInfo* Result;
	if (FreeViewInfoSnapshots.size())
	{
		Result = FreeViewInfoSnapshots.back();
		FreeViewInfoSnapshots.pop_back();
	}
	else
	{
		Result = (FViewInfo*)_aligned_malloc(sizeof(FViewInfo), alignof(FViewInfo));
	}
	memcpy(Result, this, sizeof(FViewInfo));

	// we want these to start null without a reference count, since we clear a ref later
// 	TUniformBufferPtr<FViewUniformShaderParameters> NullViewUniformBuffer;
// 	memcpy(Result->ViewUniformBuffer, NullViewUniformBuffer);
	Result->ViewUniformBuffer.reset();
// 	TUniformBufferPtr<FMobileDirectionalLightShaderParameters> NullMobileDirectionalLightUniformBuffer;
// 	for (size_t i = 0; i < ARRAY_COUNT(Result->MobileDirectionalLightUniformBuffers); i++)
// 	{
// 		// This memcpy is necessary to clear the reference from the memcpy of the whole of this -> Result without releasing the pointer
// 		FMemory::Memcpy(Result->MobileDirectionalLightUniformBuffers[i], NullMobileDirectionalLightUniformBuffer);
// 
// 		// But what we really want is the null buffer.
// 		Result->MobileDirectionalLightUniformBuffers[i] = GetNullMobileDirectionalLightShaderParameters();
// 	}

// 	TUniquePtr<FViewUniformShaderParameters> NullViewParameters;
// 	FMemory::Memcpy(Result->CachedViewUniformShaderParameters, NullViewParameters);
	Result->CachedViewUniformShaderParameters = nullptr;
	Result->bIsSnapshot = true;
	ViewInfoSnapshots.push_back(Result);
	return Result;
}

void FViewInfo::DestroyAllSnapshots()
{
	int32 NumToRemove = (int32)FreeViewInfoSnapshots.size() - (int32)(ViewInfoSnapshots.size() + 2);
	if (NumToRemove > 0)
	{
		for (int32 Index = 0; Index < NumToRemove; Index++)
		{
			_aligned_free(FreeViewInfoSnapshots[Index]);
		}
		FreeViewInfoSnapshots.erase(FreeViewInfoSnapshots.begin(), FreeViewInfoSnapshots.begin() + NumToRemove);
	}
	for (FViewInfo* Snapshot : ViewInfoSnapshots)
	{
		Snapshot->ViewUniformBuffer.reset();
		if (Snapshot->CachedViewUniformShaderParameters)
		{
			delete Snapshot->CachedViewUniformShaderParameters;
			Snapshot->CachedViewUniformShaderParameters = nullptr;
		}
		FreeViewInfoSnapshots.push_back(Snapshot);
	}
	ViewInfoSnapshots.clear();
}

void FViewInfo::Init()
{
	ViewRect = FIntRect(0, 0, 0, 0);

	CachedViewUniformShaderParameters = nullptr;
	// 	bHasNoVisiblePrimitive = false;
	// 	bHasTranslucentViewMeshElements = 0;
	// 	bPrevTransformsReset = false;
	// 	bIgnoreExistingQueries = false;
	// 	bDisableQuerySubmissions = false;
	// 	bDisableDistanceBasedFadeTransitions = false;
	// 	ShadingModelMaskInView = 0;
	// 
	// 	NumVisibleStaticMeshElements = 0;
	// 	PrecomputedVisibilityData = 0;
	// 	bSceneHasDecals = 0;
	// 
	// 	bIsViewInfo = true;

	//bUsesGlobalDistanceField = false;
	// 	bUsesLightingChannels = false;
	// 	bTranslucentSurfaceLighting = false;
	//bUsesSceneDepth = false;

	ExponentialFogParameters = Vector4(0, 1, 1, 0);
	ExponentialFogColor = FVector::ZeroVector;
	FogMaxOpacity = 1;
	ExponentialFogParameters3 = Vector4(0, 0, 0, 0);
	SinCosInscatteringColorCubemapRotation = Vector2(0, 0);
	FogInscatteringColorCubemap = NULL;
	FogInscatteringTextureParameters = FVector::ZeroVector;
	// 
	bUseDirectionalInscattering = false;
	DirectionalInscatteringExponent = 0;
	DirectionalInscatteringStartDistance = 0;
	InscatteringLightDirection = FVector(0);
	DirectionalInscatteringColor = FLinearColor();

	// 	for (int32 CascadeIndex = 0; CascadeIndex < TVC_MAX; CascadeIndex++)
	// 	{
	// 		TranslucencyLightingVolumeMin[CascadeIndex] = FVector(0);
	// 		TranslucencyVolumeVoxelSize[CascadeIndex] = 0;
	// 		TranslucencyLightingVolumeSize[CascadeIndex] = FVector(0);
	// 	}

	// 	const int32 MaxMobileShadowCascadeCount = FMath::Clamp(CVarMaxMobileShadowCascades.GetValueOnAnyThread(), 0, MAX_MOBILE_SHADOWCASCADES);
	// 	const int32 MaxShadowCascadeCountUpperBound = GetFeatureLevel() >= ERHIFeatureLevel::SM4 ? 10 : MaxMobileShadowCascadeCount;
	// 
	// 	MaxShadowCascades = FMath::Clamp<int32>(CVarMaxShadowCascades.GetValueOnAnyThread(), 0, MaxShadowCascadeCountUpperBound);

	ShaderMap = GetGlobalShaderMap();

	// 	ViewState = (FSceneViewState*)State;
	bIsSnapshot = false;
	// 
	// 	bAllowStencilDither = false;
	// 
	// 	ForwardLightingResources = nullptr;
	// 
	// 	NumBoxReflectionCaptures = 0;
	// 	NumSphereReflectionCaptures = 0;
	// 	FurthestReflectionCaptureDistance = 0;
	// 
	// 	// Disable HDR encoding for editor elements.
	// 	EditorSimpleElementCollector.BatchedElements.EnableMobileHDREncoding(false);
	// 
	// 	TemporalJitterPixels = FVector2D::ZeroVector;

	// 	PreExposure = 1.0f;
	// 	MaterialTextureMipBias = 0.0f;

	// Cache TEXTUREGROUP_World's for the render thread to create the material textures' shared sampler.
	// 	if (IsInGameThread())
	// 	{
	// 		WorldTextureGroupSamplerFilter = (ESamplerFilter)UDeviceProfileManager::Get().GetActiveProfile()->GetTextureLODSettings()->GetSamplerFilter(TEXTUREGROUP_World);
	// 		bIsValidWorldTextureGroupSamplerFilter = true;
	// 	}
	// 	else
	// 	{
	// 		bIsValidWorldTextureGroupSamplerFilter = false;
	// 	}

}

void FViewInfo::SetupSkyIrradianceEnvironmentMapConstants(Vector4* OutSkyIrradianceEnvironmentMap) const
{
	FScene* Scene = nullptr;

	if (Family->Scene)
	{
		Scene = Family->Scene;
	}

	if (Scene
		&& Scene->SkyLight
		// Skylights with static lighting already had their diffuse contribution baked into lightmaps
		&& !Scene->SkyLight->bHasStaticLighting
		/*&& Family->EngineShowFlags.SkyLighting*/)
	{
		const FSHVectorRGB3& SkyIrradiance = Scene->SkyLight->IrradianceEnvironmentMap;

		const float SqrtPI = FMath::Sqrt(PI);
		const float Coefficient0 = 1.0f / (2 * SqrtPI);
		const float Coefficient1 = FMath::Sqrt(3) / (3 * SqrtPI);
		const float Coefficient2 = FMath::Sqrt(15) / (8 * SqrtPI);
		const float Coefficient3 = FMath::Sqrt(5) / (16 * SqrtPI);
		const float Coefficient4 = .5f * Coefficient2;

		// Pack the SH coefficients in a way that makes applying the lighting use the least shader instructions
		// This has the diffuse convolution coefficients baked in
		// See "Stupid Spherical Harmonics (SH) Tricks"
		OutSkyIrradianceEnvironmentMap[0].X = -Coefficient1 * SkyIrradiance.R.V[3];
		OutSkyIrradianceEnvironmentMap[0].Y = -Coefficient1 * SkyIrradiance.R.V[1];
		OutSkyIrradianceEnvironmentMap[0].Z = Coefficient1 * SkyIrradiance.R.V[2];
		OutSkyIrradianceEnvironmentMap[0].W = Coefficient0 * SkyIrradiance.R.V[0] - Coefficient3 * SkyIrradiance.R.V[6];

		OutSkyIrradianceEnvironmentMap[1].X = -Coefficient1 * SkyIrradiance.G.V[3];
		OutSkyIrradianceEnvironmentMap[1].Y = -Coefficient1 * SkyIrradiance.G.V[1];
		OutSkyIrradianceEnvironmentMap[1].Z = Coefficient1 * SkyIrradiance.G.V[2];
		OutSkyIrradianceEnvironmentMap[1].W = Coefficient0 * SkyIrradiance.G.V[0] - Coefficient3 * SkyIrradiance.G.V[6];

		OutSkyIrradianceEnvironmentMap[2].X = -Coefficient1 * SkyIrradiance.B.V[3];
		OutSkyIrradianceEnvironmentMap[2].Y = -Coefficient1 * SkyIrradiance.B.V[1];
		OutSkyIrradianceEnvironmentMap[2].Z = Coefficient1 * SkyIrradiance.B.V[2];
		OutSkyIrradianceEnvironmentMap[2].W = Coefficient0 * SkyIrradiance.B.V[0] - Coefficient3 * SkyIrradiance.B.V[6];

		OutSkyIrradianceEnvironmentMap[3].X = Coefficient2 * SkyIrradiance.R.V[4];
		OutSkyIrradianceEnvironmentMap[3].Y = -Coefficient2 * SkyIrradiance.R.V[5];
		OutSkyIrradianceEnvironmentMap[3].Z = 3 * Coefficient3 * SkyIrradiance.R.V[6];
		OutSkyIrradianceEnvironmentMap[3].W = -Coefficient2 * SkyIrradiance.R.V[7];

		OutSkyIrradianceEnvironmentMap[4].X = Coefficient2 * SkyIrradiance.G.V[4];
		OutSkyIrradianceEnvironmentMap[4].Y = -Coefficient2 * SkyIrradiance.G.V[5];
		OutSkyIrradianceEnvironmentMap[4].Z = 3 * Coefficient3 * SkyIrradiance.G.V[6];
		OutSkyIrradianceEnvironmentMap[4].W = -Coefficient2 * SkyIrradiance.G.V[7];

		OutSkyIrradianceEnvironmentMap[5].X = Coefficient2 * SkyIrradiance.B.V[4];
		OutSkyIrradianceEnvironmentMap[5].Y = -Coefficient2 * SkyIrradiance.B.V[5];
		OutSkyIrradianceEnvironmentMap[5].Z = 3 * Coefficient3 * SkyIrradiance.B.V[6];
		OutSkyIrradianceEnvironmentMap[5].W = -Coefficient2 * SkyIrradiance.B.V[7];

		OutSkyIrradianceEnvironmentMap[6].X = Coefficient4 * SkyIrradiance.R.V[8];
		OutSkyIrradianceEnvironmentMap[6].Y = Coefficient4 * SkyIrradiance.G.V[8];
		OutSkyIrradianceEnvironmentMap[6].Z = Coefficient4 * SkyIrradiance.B.V[8];
		OutSkyIrradianceEnvironmentMap[6].W = 1;
	}
	else
	{
		memset(OutSkyIrradianceEnvironmentMap,0, sizeof(Vector4) * 7);
	}
}


FSceneRenderer::FSceneRenderer(FSceneViewFamily& InViewFamily)
	:Scene(InViewFamily.Scene),
	ViewFamily(InViewFamily)
{
	for (uint32 ViewIndex = 0; ViewIndex < InViewFamily.Views.size(); ViewIndex++)
	{
		FViewInfo View(InViewFamily.Views[ViewIndex]);
		Views.emplace_back(View);
	}
	EarlyZPassMode = DDM_AllOpaque;

}


void FSceneRenderer::PrepareViewRectsForRendering()
{
	for (FViewInfo& View : Views)
	{
		View.ViewRect = View.UnscaledViewRect;
	}
}


void FSceneRenderer::Render()
{
	PrepareViewRectsForRendering();

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

	SceneContext.ReleaseSceneColor();

	SceneContext.Allocate(this);
	SceneContext.AllocGBufferTargets();

	SCOPED_DRAW_EVENT(Scene);

	InitViews();
	RenderPrePass();
	RenderHzb();
	RenderShadowDepthMaps();
	// Use readonly depth in the base pass if we have a full depth prepass
	const bool bAllowReadonlyDepthBasePass = EarlyZPassMode == DDM_AllOpaque
		//&& CVarBasePassWriteDepthEvenWithFullPrepass.GetValueOnRenderThread() == 0
		//&& !ViewFamily.EngineShowFlags.ShaderComplexity
		//&& !ViewFamily.UseDebugViewPS()
		//&& !bIsWireframe
		/*&& !ViewFamily.EngineShowFlags.LightMapDensity*/;

	const FExclusiveDepthStencil::Type BasePassDepthStencilAccess =
		bAllowReadonlyDepthBasePass
		? FExclusiveDepthStencil::DepthRead_StencilWrite
		: FExclusiveDepthStencil::DepthWrite_StencilWrite;

	SceneContext.BeginRenderingGBuffer(ERenderTargetLoadAction::EClear, ERenderTargetLoadAction::ELoad, BasePassDepthStencilAccess);
	RenderBasePass(BasePassDepthStencilAccess,nullptr);
	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		GCompositionLighting.ProcessAfterBasePass(Views[ViewIndex]);
	}
	RenderLights();

	FLightShaftsOutput LightShaftOutput;
	//RenderLightShaftOcclusion(RHICmdList, LightShaftOutput);

	if (ShouldRenderAtmosphere(ViewFamily))
	{
		if (Scene->AtmosphericFog)
		{
			// Update RenderFlag based on LightShaftTexture is valid or not
			if (LightShaftOutput.LightShaftOcclusion)
			{
				Scene->AtmosphericFog->RenderFlag &= EAtmosphereRenderFlag::E_LightShaftMask;
			}
			else
			{
				Scene->AtmosphericFog->RenderFlag |= EAtmosphereRenderFlag::E_DisableLightShaft;
			}
			RenderAtmosphere(LightShaftOutput);
		}
	}

}

FIntPoint FSceneRenderer::GetDesiredInternalBufferSize(const FSceneViewFamily& ViewFamily)
{
	// If not supporting screen percentage, bypass all computation.
	//if (!ViewFamily.SupportsScreenPercentage())
	{
		FIntPoint FamilySizeUpperBound(0, 0);

		for (const FSceneView* View : ViewFamily.Views)
		{
			FamilySizeUpperBound.X = FMath::Max(FamilySizeUpperBound.X, View->UnscaledViewRect.Max.X);
			FamilySizeUpperBound.Y = FMath::Max(FamilySizeUpperBound.Y, View->UnscaledViewRect.Max.Y);
		}

		FIntPoint DesiredBufferSize;
		QuantizeSceneBufferSize(FamilySizeUpperBound, DesiredBufferSize);
		return DesiredBufferSize;
	}
}

void FSceneRenderer::UpdatePrimitivePrecomputedLightingBuffers()
{
	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		FViewInfo& View = Views[ViewIndex];

		for (uint32 Index = 0; Index < View.DirtyPrecomputedLightingBufferPrimitives.size(); ++Index)
		{
			FPrimitiveSceneInfo* PrimitiveSceneInfo = View.DirtyPrecomputedLightingBufferPrimitives[Index];
			PrimitiveSceneInfo->UpdatePrecomputedLightingBuffer();
		}
	}
}

void FSceneRenderer::ClearPrimitiveSingleFramePrecomputedLightingBuffers()
{

}

