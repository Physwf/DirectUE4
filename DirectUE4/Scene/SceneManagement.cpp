#include "SceneManagement.h"
#include "LightComponent.h"

FLightSceneProxy::FLightSceneProxy(const ULightComponent* InLightComponent)
	: LightComponent(InLightComponent)
	, ShadowBias(InLightComponent->ShadowBias)
	, bStaticLighting(InLightComponent->HasStaticLighting())
	, bStaticShadowing(InLightComponent->HasStaticShadowing())
	, bCastDynamicShadow(InLightComponent->CastShadows && InLightComponent->CastDynamicShadows)
	, bCastStaticShadow(InLightComponent->CastShadows && InLightComponent->CastStaticShadows)
	, bCastTranslucentShadows(InLightComponent->CastTranslucentShadows)
	, LightType(InLightComponent->GetLightType())
{

}

bool FLightSceneProxy::ShouldCreatePerObjectShadowsForDynamicObjects() const
{
	return HasStaticShadowing() && !HasStaticLighting();
}

bool FLightSceneProxy::UseCSMForDynamicObjects() const
{
	return false;
}

void FLightSceneProxy::SetTransform(const FMatrix& InLightToWorld, const Vector4& InPosition)
{
	LightToWorld = InLightToWorld;
	WorldToLight = InLightToWorld.InverseFast();
	Position = InPosition;
}

ELightInteractionType FLightCacheInterface::GetStaticInteraction(const FLightSceneProxy* LightSceneProxy, const std::vector<uint32>& IrrelevantLights) const
{

}

FLightMapInteraction FLightCacheInterface::GetLightMapInteraction() const
{
	if (bGlobalVolumeLightmap)
	{
		return FLightMapInteraction::GlobalVolume();
	}

	return LightMap ? LightMap->GetInteraction() : FLightMapInteraction();
}

FShadowMapInteraction FLightCacheInterface::GetShadowMapInteraction() const
{
	if (bGlobalVolumeLightmap)
	{
		return FShadowMapInteraction::GlobalVolume();
	}

	return ShadowMap ? ShadowMap->GetInteraction() : FShadowMapInteraction();
}

FLightMapInteraction FLightMapInteraction::Texture(
	ID3D11ShaderResourceView** InTextures, 
	ID3D11Texture2D* InSkyOcclusionTexture, 
	ID3D11Texture2D* InAOMaterialMaskTexture, 
	const Vector4* InCoefficientScales, 
	const Vector4* InCoefficientAdds, 
	const Vector2& InCoordinateScale, 
	const Vector2& InCoordinateBias, 
	bool bUseHighQualityLightMaps)
{
	FLightMapInteraction Result;
	Result.Type = LMIT_Texture;

#if ALLOW_LQ_LIGHTMAPS && ALLOW_HQ_LIGHTMAPS
	// however, if simple and directional are allowed, then we must use the value passed in,
	// and then cache the number as well
	Result.bAllowHighQualityLightMaps = bUseHighQualityLightMaps;
	if (bUseHighQualityLightMaps)
	{
		Result.NumLightmapCoefficients = NUM_HQ_LIGHTMAP_COEF;
	}
	else
	{
		Result.NumLightmapCoefficients = NUM_LQ_LIGHTMAP_COEF;
	}
#endif

	//copy over the appropriate textures and scales
	if (bUseHighQualityLightMaps)
	{
#if ALLOW_HQ_LIGHTMAPS
		Result.HighQualityTexture = InTextures[0];
		Result.SkyOcclusionTexture = InSkyOcclusionTexture;
		Result.AOMaterialMaskTexture = InAOMaterialMaskTexture;
		for (uint32 CoefficientIndex = 0; CoefficientIndex < NUM_HQ_LIGHTMAP_COEF; CoefficientIndex++)
		{
			Result.HighQualityCoefficientScales[CoefficientIndex] = InCoefficientScales[CoefficientIndex];
			Result.HighQualityCoefficientAdds[CoefficientIndex] = InCoefficientAdds[CoefficientIndex];
		}
#endif
	}

	Result.CoordinateScale = InCoordinateScale;
	Result.CoordinateBias = InCoordinateBias;
	return Result;
}
