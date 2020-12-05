#include "SceneManagement.h"
#include "LightComponent.h"
#include "LightMap.h"
#include "ShadowMap.h"

FLightSceneProxy::FLightSceneProxy(const ULightComponent* InLightComponent)
	: LightComponent(InLightComponent)
	, ShadowBias(InLightComponent->ShadowBias)
	, ShadowSharpen(InLightComponent->ShadowSharpen)
	, ContactShadowLength(InLightComponent->ContactShadowLength)
	, bContactShadowLengthInWS(InLightComponent->ContactShadowLengthInWS ? true : false)
	, SpecularScale(InLightComponent->SpecularScale)
	//, bMovable(InLightComponent->IsMovable())
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
	if (bGlobalVolumeLightmap)
	{
		if (LightSceneProxy->HasStaticLighting())
		{
			return LIT_CachedLightMap;
		}
		else if (LightSceneProxy->HasStaticShadowing())
		{
			return LIT_CachedSignedDistanceFieldShadowMap2D;
		}
		else
		{
			return LIT_MAX;
		}
	}

	ELightInteractionType Ret = LIT_MAX;

	// Check if the light has static lighting or shadowing.
	if (LightSceneProxy->HasStaticShadowing())
	{
		const uint32 LightGuid = LightSceneProxy->GetLightGuid();

		if (std::find(IrrelevantLights.begin(),IrrelevantLights.end(),LightGuid) != IrrelevantLights.end())
		{
			Ret = LIT_CachedIrrelevant;
		}
		else if (LightMap && LightMap->ContainsLight(LightGuid))
		{
			Ret = LIT_CachedLightMap;
		}
		else if (ShadowMap && ShadowMap->ContainsLight(LightGuid))
		{
			Ret = LIT_CachedSignedDistanceFieldShadowMap2D;
		}
	}

	return Ret;
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
	ID3D11ShaderResourceView* const* InTextures,
	ID3D11ShaderResourceView* InSkyOcclusionTexture,
	ID3D11ShaderResourceView* InAOMaterialMaskTexture,
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
