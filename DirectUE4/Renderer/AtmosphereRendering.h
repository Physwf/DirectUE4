#pragma once

#include "D3D11RHI.h"
#include "UnrealMath.h"

class Scene;

class AtmosphericFogSceneInfo
{
public:
	/** The fog component the scene info is for. */
	//const UAtmosphericFogComponent* Component;
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
	LinearColor DefaultSunColor;
	Vector DefaultSunDirection;
	uint32 RenderFlag;
	uint32 InscatterAltitudeSampleNum;
	//class FAtmosphereTextureResource* TransmittanceResource;
	//class FAtmosphereTextureResource* IrradianceResource;
	//class FAtmosphereTextureResource* InscatterResource;
	
	/** Initialization constructor. */
	explicit AtmosphericFogSceneInfo(/*UAtmosphericFogComponent* InComponent,*/ const class Scene* InScene);
	~AtmosphericFogSceneInfo();


};

bool ShouldRenderAtmosphere(/*const FSceneViewFamily& Family*/);
//void InitAtmosphereConstantsInView(FViewInfo& View);

void InitAtomosphereFog();
