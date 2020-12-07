#include "AtmosphereFog.h"
#include "DirectXTex.h"
#include "World.h"
#include "Scene.h"

using namespace DirectX;

FAtmospherePrecomputeParameters::FAtmospherePrecomputeParameters()
{
	DensityHeight = 0.5f;
	MaxScatteringOrder = 4;
	TransmittanceTexWidth = 256;
	TransmittanceTexHeight = 64;
	IrradianceTexWidth = 64;
	IrradianceTexHeight	= 16;
	InscatterAltitudeSampleNum = 32;
	InscatterMuNum = 128;
	InscatterMuSNum = 32;
	InscatterNuNum = 8;
}

UAtmosphericFogComponent::UAtmosphericFogComponent(AActor* InOwner)
	: USceneComponent(InOwner)
{
	SunMultiplier = 1.0f;
	FogMultiplier = 1.0f;
	DensityMultiplier = 1.0f;
	DensityOffset = 0.0f;
	DistanceScale = 1.0f;
	AltitudeScale = 1.0f;
	GroundOffset = -98975.89844f;
	DistanceScale = 1.0f;
	AltitudeScale = 1.0f;
	DistanceOffset = 0.0f;
	GroundOffset = -100000.0f;
	StartDistance = 15000.0f;
	SunDiscScale = 1.0f;
	DefaultBrightness = 50.0f;
	DefaultLightColor = FColor{ 255 , 255 , 255 };
	bDisableSunDisk = 0;
	bDisableGroundScattering = 0;

	{
		TexMetadata Metadata;
		ScratchImage sImage;
		if (S_OK == LoadFromDDSFile(TEXT("./dx11demo/AtmosphereTransmittanceTexture.dds"), DDS_FLAGS_NONE, &Metadata, sImage))
		{
			CreateShaderResourceView(D3D11Device, sImage.GetImages(), 1, Metadata, &TransmittanceResource);
		}
	}
	{
		TexMetadata Metadata;
		ScratchImage sImage;
		if (S_OK == LoadFromDDSFile(TEXT("./dx11demo/AtmosphereIrradianceTexture.dds"), DDS_FLAGS_NONE, &Metadata, sImage))
		{
			CreateShaderResourceView(D3D11Device, sImage.GetImages(), 1, Metadata, &IrradianceResource);
		}
	}
	{
		TexMetadata Metadata;
		ScratchImage sImage;
		if (S_OK == LoadFromDDSFile(TEXT("./dx11demo/AtmosphereInscatterTexture.dds"), DDS_FLAGS_NONE, &Metadata, sImage))
		{
			CreateShaderResourceView(D3D11Device, sImage.GetImages(), 32, Metadata, &InscatterResource);
		}
	}
}

UAtmosphericFogComponent::~UAtmosphericFogComponent()
{

}

void UAtmosphericFogComponent::CreateRenderState_Concurrent()
{
	USceneComponent::CreateRenderState_Concurrent();
	AddFogIfNeeded();
}

void UAtmosphericFogComponent::SendRenderTransform_Concurrent()
{
	GetWorld()->Scene->RemoveAtmosphericFog(this);
	AddFogIfNeeded();
	USceneComponent::SendRenderTransform_Concurrent();
}

void UAtmosphericFogComponent::DestroyRenderState_Concurrent()
{
	USceneComponent::DestroyRenderState_Concurrent();
	GetWorld()->Scene->RemoveAtmosphericFog(this);
}

void UAtmosphericFogComponent::AddFogIfNeeded()
{
	GetWorld()->Scene->AddAtmosphericFog(this);
}

AAtmosphericFog::AAtmosphericFog(UWorld* InOwner)
	: AActor(InOwner)
{
	AtmosphericFogComponent = new UAtmosphericFogComponent(this);

}

void AAtmosphericFog::Tick(float fDeltaSeconds)
{

}

void AAtmosphericFog::PostLoad()
{
	AtmosphericFogComponent->Register();
}

