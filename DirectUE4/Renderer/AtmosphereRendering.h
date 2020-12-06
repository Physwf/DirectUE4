#pragma once

#include "D3D11RHI.h"
#include "UnrealMath.h"

class FScene;
class UAtmosphericFogComponent;
class FSceneViewFamily;
class FViewInfo;

namespace EAtmosphereRenderFlag
{
	enum Type
	{
		E_EnableAll = 0,
		E_DisableSunDisk = 1,
		E_DisableGroundScattering = 2,
		E_DisableLightShaft = 4, // Light Shaft shadow
		E_DisableSunAndGround = E_DisableSunDisk | E_DisableGroundScattering,
		E_DisableSunAndLightShaft = E_DisableSunDisk | E_DisableLightShaft,
		E_DisableGroundAndLightShaft = E_DisableGroundScattering | E_DisableLightShaft,
		E_DisableAll = E_DisableSunDisk | E_DisableGroundScattering | E_DisableLightShaft,
		E_RenderFlagMax = E_DisableAll + 1,
		E_LightShaftMask = (~E_DisableLightShaft),
	};
}

class FAtmosphericFogSceneInfo
{
public:
	/** The fog component the scene info is for. */
	const UAtmosphericFogComponent* Component;

	float SunMultiplier;
	float FogMultiplier;
	float InvDensityMultiplier;
	float DensityOffset;
	float GroundOffset;
	float DistanceScale;
	float AltitudeScale;
	float RHeight;
	float StartDistance;
	float DistanceOffset;
	float SunDiscScale;
	FLinearColor DefaultSunColor;
	FVector DefaultSunDirection;
	uint32 RenderFlag;
	uint32 InscatterAltitudeSampleNum;
	ID3D11ShaderResourceView* TransmittanceResource;
	ID3D11ShaderResourceView* IrradianceResource;
	ID3D11ShaderResourceView* InscatterResource;
	
	/** Initialization constructor. */
	explicit FAtmosphericFogSceneInfo(UAtmosphericFogComponent* InComponent, const class FScene* InScene);
	~FAtmosphericFogSceneInfo();
};

bool ShouldRenderAtmosphere(const FSceneViewFamily& Family);
void InitAtmosphereConstantsInView(FViewInfo& View);

