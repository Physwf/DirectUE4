#pragma once

#include "EngineTypes.h"
#include "LightComponent.h"
#include "Actor.h"
#include "SHMath.h"

enum ESkyLightSourceType
{
	/** Construct the sky light from the captured scene, anything further than SkyDistanceThreshold from the sky light position will be included. */
	SLS_CapturedScene,
	/** Construct the sky light from the specified cubemap. */
	SLS_SpecifiedCubemap,
	SLS_MAX,
};

class FSkyLightSceneProxy;

class USkyLightComponent : public ULightComponentBase
{
public:
	USkyLightComponent(AActor* InOwner);

	/** Indicates where to get the light contribution from. */
	enum ESkyLightSourceType SourceType;

	/** Cubemap to use for sky lighting if SourceType is set to SLS_SpecifiedCubemap. */
	std::shared_ptr<FD3D11Texture2D> Cubemap;

	/** Angle to rotate the source cubemap when SourceType is set to SLS_SpecifiedCubemap. */
	float SourceCubemapAngle;

	/** Maximum resolution for the very top processed cubemap mip. Must be a power of 2. */
	int32 CubemapResolution;

	/**
	* Distance from the sky light at which any geometry should be treated as part of the sky.
	* This is also used by reflection captures, so update reflection captures to see the impact.
	*/
	float SkyDistanceThreshold;

	/** Only capture emissive materials. Skips all lighting making the capture cheaper. Recomended when using CaptureEveryFrame */
	bool bCaptureEmissiveOnly;

	/**
	* Whether all distant lighting from the lower hemisphere should be set to LowerHemisphereColor.
	* Enabling this is accurate when lighting a scene on a planet where the ground blocks the sky,
	* However disabling it can be useful to approximate skylight bounce lighting (eg Movable light).
	*/
	bool bLowerHemisphereIsBlack;

	FLinearColor LowerHemisphereColor;

	/**
	* Max distance that the occlusion of one point will affect another.
	* Higher values increase the cost of Distance Field AO exponentially.
	*/
	float OcclusionMaxDistance;

	/**
	* Contrast S-curve applied to the computed AO.  A value of 0 means no contrast increase, 1 is a significant contrast increase.
	*/
	float Contrast;

	/**
	* Exponent applied to the computed AO.  Values lower than 1 brighten occlusion overall without losing contact shadows.
	*/
	float OcclusionExponent;

	/**
	* Controls the darkest that a fully occluded area can get.  This tends to destroy contact shadows, use Contrast or OcclusionExponent instead.
	*/
	float MinOcclusion;

	/** Tint color on occluded areas, artistic control. */
	FColor OcclusionTint;

	/** Controls how occlusion from Distance Field Ambient Occlusion is combined with Screen Space Ambient Occlusion. */
	enum EOcclusionCombineMode OcclusionCombineMode;

	class FSkyLightSceneProxy* CreateSceneProxy() const;

	static void UpdateSkyCaptureContents(UWorld* WorldToUpdate);
	static void UpdateSkyCaptureContentsArray(UWorld* WorldToUpdate, std::vector<USkyLightComponent*>& ComponentArray, bool bBlendSources);
protected:
	FSkyLightSceneProxy * SceneProxy;

	std::shared_ptr<FD3D11Texture2D> ProcessedSkyTexture;
	FSHVectorRGB3 IrradianceEnvironmentMap;
	float AverageBrightness;

	ID3D11ShaderResourceView* BlendDestinationCubemap;
	ComPtr<ID3D11ShaderResourceView> BlendDestinationProcessedSkyTexture;
	FSHVectorRGB3 BlendDestinationIrradianceEnvironmentMap;

	static std::vector<USkyLightComponent*> SkyCapturesToUpdate;
	static std::vector<USkyLightComponent*> SkyCapturesToUpdateBlendDestinations;

	virtual void OnRegister() override;

	virtual void CreateRenderState_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;

	friend class FSkyLightSceneProxy;
};

class ASkylight : public AActor
{
public:
	ASkylight(class UWorld* InOwner);

	virtual void Tick(float fDeltaSeconds) override {};

	virtual void PostLoad() override;
private:
	USkyLightComponent* SkylightComponent;
};