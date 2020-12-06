#include "FogRendering.h"
#include "DeferredShading.h"
#include "AtmosphereRendering.h"

void SetupFogUniformParameters(const class FViewInfo& View, FFogUniformParameters& OutParameters)
{
	// Exponential Height Fog
	{
		ID3D11ShaderResourceView* Cubemap = GWhiteTextureCubeSRV;

		if (View.FogInscatteringColorCubemap)
		{
			Cubemap = View.FogInscatteringColorCubemap;
		}

		OutParameters.Constants.ExponentialFogParameters = View.ExponentialFogParameters;
		OutParameters.Constants.ExponentialFogColorParameter = Vector4(View.ExponentialFogColor, 1.0f - View.FogMaxOpacity);
		OutParameters.Constants.ExponentialFogParameters3 = View.ExponentialFogParameters3;
		OutParameters.Constants.SinCosInscatteringColorCubemapRotation = View.SinCosInscatteringColorCubemapRotation;
		OutParameters.Constants.FogInscatteringTextureParameters = View.FogInscatteringTextureParameters;
		OutParameters.Constants.InscatteringLightDirection = Vector4(View.InscatteringLightDirection, View.bUseDirectionalInscattering ? 1.f : 0);
		OutParameters.Constants.DirectionalInscatteringColor = Vector4(FVector(View.DirectionalInscatteringColor), FMath::Clamp(View.DirectionalInscatteringExponent, 0.000001f, 1000.0f));
		OutParameters.Constants.DirectionalInscatteringStartDistance = View.DirectionalInscatteringStartDistance;
		OutParameters.FogInscatteringColorCubemap = Cubemap;
		OutParameters.FogInscatteringColorSampler = TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI();
	}

	// Volumetric Fog
	{
		ID3D11ShaderResourceView* IntegratedLightScatteringTexture = nullptr;

		if (View.VolumetricFogResources.IntegratedLightScattering)
		{
			IntegratedLightScatteringTexture = View.VolumetricFogResources.IntegratedLightScattering->ShaderResourceTexture->GetShaderResourceView();
		}
		if (!IntegratedLightScatteringTexture)
		{
			IntegratedLightScatteringTexture = GBlackVolumeTextureSRV;
		}
		//SetBlack3DIfNull(IntegratedLightScatteringTexture);

		const bool bApplyVolumetricFog = View.VolumetricFogResources.IntegratedLightScattering != NULL;
		OutParameters.Constants.ApplyVolumetricFog = bApplyVolumetricFog ? 1.0f : 0.0f;
		OutParameters.IntegratedLightScattering = IntegratedLightScatteringTexture;
		OutParameters.IntegratedLightScatteringSampler = TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI();
	}
}

bool ShouldRenderFog(const class FSceneViewFamily& Family)
{
	return false;
}

void FSceneRenderer::InitFogConstants()
{
	// console command override
	float FogDensityOverride = -1.0f;
	float FogStartDistanceOverride = -1.0f;

	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		FViewInfo& View = Views[ViewIndex];
		InitAtmosphereConstantsInView(View);
		// set fog consts based on height fog components
		/*
		if (ShouldRenderFog(*View.Family))
		{
		if (Scene->ExponentialFogs.Num() > 0)
		{
		const FExponentialHeightFogSceneInfo& FogInfo = Scene->ExponentialFogs[0];
		const float CosTerminatorAngle = FMath::Clamp(FMath::Cos(FogInfo.LightTerminatorAngle * PI / 180.0f), -1.0f + DELTA, 1.0f - DELTA);
		const float CollapsedFogParameterPower = FMath::Clamp(
		-FogInfo.FogHeightFalloff * (View.ViewMatrices.GetViewOrigin().Z - FogInfo.FogHeight),
		-126.f + 1.f, // min and max exponent values for IEEE floating points (http://en.wikipedia.org/wiki/IEEE_floating_point)
		+127.f - 1.f
		);
		const float CollapsedFogParameter = FogInfo.FogDensity * FMath::Pow(2.0f, CollapsedFogParameterPower);
		View.ExponentialFogParameters = FVector4(CollapsedFogParameter, FogInfo.FogHeightFalloff, CosTerminatorAngle, FogInfo.StartDistance);
		View.ExponentialFogColor = FVector(FogInfo.FogColor.R, FogInfo.FogColor.G, FogInfo.FogColor.B);
		View.FogMaxOpacity = FogInfo.FogMaxOpacity;
		View.ExponentialFogParameters3 = FVector4(FogInfo.FogDensity, FogInfo.FogHeight, FogInfo.InscatteringColorCubemap ? 1.0f : 0.0f, FogInfo.FogCutoffDistance);
		View.SinCosInscatteringColorCubemapRotation = FVector2D(FMath::Sin(FogInfo.InscatteringColorCubemapAngle), FMath::Cos(FogInfo.InscatteringColorCubemapAngle));
		View.FogInscatteringColorCubemap = FogInfo.InscatteringColorCubemap;
		const float InvRange = 1.0f / FMath::Max(FogInfo.FullyDirectionalInscatteringColorDistance - FogInfo.NonDirectionalInscatteringColorDistance, .00001f);
		float NumMips = 1.0f;

		if (FogInfo.InscatteringColorCubemap)
		{
		NumMips = FogInfo.InscatteringColorCubemap->GetNumMips();
		}

		View.FogInscatteringTextureParameters = FVector(InvRange, -FogInfo.NonDirectionalInscatteringColorDistance * InvRange, NumMips);

		View.DirectionalInscatteringExponent = FogInfo.DirectionalInscatteringExponent;
		View.DirectionalInscatteringStartDistance = FogInfo.DirectionalInscatteringStartDistance;
		View.bUseDirectionalInscattering = false;
		View.InscatteringLightDirection = FVector(0);

		for (TSparseArray<FLightSceneInfoCompact>::TConstIterator It(Scene->Lights); It; ++It)
		{
		const FLightSceneInfoCompact& LightInfo = *It;

		// This will find the first directional light that is set to be used as an atmospheric sun light of sufficient brightness.
		// If you have more than one directional light with these properties then all subsequent lights will be ignored.
		if (LightInfo.LightSceneInfo->Proxy->GetLightType() == LightType_Directional
		&& LightInfo.LightSceneInfo->Proxy->IsUsedAsAtmosphereSunLight()
		&& LightInfo.LightSceneInfo->Proxy->GetColor().ComputeLuminance() > KINDA_SMALL_NUMBER
		&& FogInfo.DirectionalInscatteringColor.ComputeLuminance() > KINDA_SMALL_NUMBER)
		{
		View.InscatteringLightDirection = -LightInfo.LightSceneInfo->Proxy->GetDirection();
		View.bUseDirectionalInscattering = true;
		View.DirectionalInscatteringColor = FogInfo.DirectionalInscatteringColor * LightInfo.LightSceneInfo->Proxy->GetColor().ComputeLuminance();
		break;
		}
		}
		}
		}
		*/
	}
}
