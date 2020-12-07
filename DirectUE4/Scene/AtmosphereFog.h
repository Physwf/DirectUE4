#pragma once

#include "SceneComponent.h"
#include "Actor.h"
#include "D3D11RHI.h"

struct FAtmospherePrecomputeParameters
{
	/** Rayleigh scattering density height scale, ranges from [0...1] */
	float DensityHeight;
	float DecayHeight_DEPRECATED;
	/** Maximum scattering order */
	int32 MaxScatteringOrder;
	/** Transmittance Texture Width */
	int32 TransmittanceTexWidth;
	/** Transmittance Texture Height */
	int32 TransmittanceTexHeight;
	/** Irradiance Texture Width */
	int32 IrradianceTexWidth;
	/** Irradiance Texture Height */
	int32 IrradianceTexHeight;
	/** Number of different altitudes at which to sample inscatter color (size of 3D texture Z dimension)*/
	int32 InscatterAltitudeSampleNum;
	/** Inscatter Texture Height */
	int32 InscatterMuNum;
	/** Inscatter Texture Width */
	int32 InscatterMuSNum;

	int32 InscatterNuNum;
	FAtmospherePrecomputeParameters();
};

class UAtmosphericFogComponent : public USceneComponent
{
public:
	UAtmosphericFogComponent(AActor* InOwner);
	~UAtmosphericFogComponent();

	/** Global scattering factor. */
	float SunMultiplier;
	/** Scattering factor on object. */
	float FogMultiplier;
	/** Fog density control factor. */
	float DensityMultiplier;
	/** Fog density offset to control opacity [-1.f ~ 1.f]. */
	float DensityOffset;
	/** Distance scale. */
	float DistanceScale;
	/** Altitude scale (only Z scale). */
	float AltitudeScale;
	/** Distance offset, in km (to handle large distance) */
	float DistanceOffset;
	/** Ground offset. */
	float GroundOffset;
	/** Start Distance. */
	float StartDistance;
	/** Distance offset, in km (to handle large distance) */
	float SunDiscScale;
	/** Default light brightness. Used when there is no sunlight placed in the level. Unit is lumens */
	float DefaultBrightness;
	/** Default light color. Used when there is no sunlight placed in the level. */
	FColor DefaultLightColor;
	/** Disable Sun Disk rendering. */
	uint32 bDisableSunDisk : 1;
	/** Disable Color scattering from ground. */
	uint32 bDisableGroundScattering : 1;
protected:
	FAtmospherePrecomputeParameters PrecomputeParams;
	//~ Begin UActorComponent Interface.
	virtual void CreateRenderState_Concurrent() override;
	virtual void SendRenderTransform_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;
	//~ End UActorComponent Interface.
	void AddFogIfNeeded();
public:
	/** The resource for Inscatter. */
	ID3D11ShaderResourceView* TransmittanceResource;
	ID3D11ShaderResourceView* IrradianceResource;
	ID3D11ShaderResourceView* InscatterResource;

	void InitResource();
	void ReleaseResource();
private:
	friend class FAtmosphericFogSceneInfo;
};

class AAtmosphericFog : public AActor
{
public:
	AAtmosphericFog(UWorld* InOwner);

	virtual void Tick(float fDeltaSeconds) override;

	virtual void PostLoad();
private:
	/** Main fog component */
	class UAtmosphericFogComponent* AtmosphericFogComponent;
public:
	/** Returns AtmosphericFogComponent subobject **/
	class UAtmosphericFogComponent* GetAtmosphericFogComponent() { return AtmosphericFogComponent; }
};
