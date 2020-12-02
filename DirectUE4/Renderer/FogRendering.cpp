#include "FogRendering.h"
#include "DeferredShading.h"

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

