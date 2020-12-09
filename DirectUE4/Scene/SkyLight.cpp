#include "SkyLight.h"
#include "SceneManagement.h"
#include "World.h"
#include "Scene.h"

USkyLightComponent::USkyLightComponent(AActor* InOwner)
	: ULightComponentBase(InOwner)
{
	SourceType = SLS_CapturedScene;
	//Cubemap;
	SourceCubemapAngle = 0;
	CubemapResolution = 128;
	SkyDistanceThreshold = 150000.000f;
	bCaptureEmissiveOnly = false;
	bLowerHemisphereIsBlack = true;
	LowerHemisphereColor = FLinearColor::Black;
	OcclusionMaxDistance = 1000.f;
	Contrast = 0;
	OcclusionExponent = 1.0f;
	MinOcclusion = 0;
	OcclusionTint = FColor(0,0,0);
	OcclusionCombineMode = OCM_Minimum;

	Intensity = 1;
	IndirectLightingIntensity = 1.0f;
	Mobility = EComponentMobility::Stationary;
	bCastVolumetricShadow = true;

	BlendFraction = 0;
	BlendDestinationAverageBrightness = 0;
}

class FSkyLightSceneProxy* USkyLightComponent::CreateSceneProxy() const
{
	if (ProcessedSkyTexture)
	{
		return new FSkyLightSceneProxy(this);
	}

	return NULL;
}


void FSkyLightSceneProxy::Initialize(
	float InBlendFraction,
	const FSHVectorRGB3* InIrradianceEnvironmentMap,
	const FSHVectorRGB3* BlendDestinationIrradianceEnvironmentMap,
	const float* InAverageBrightness,
	const float* BlendDestinationAverageBrightness)
{
	BlendFraction = FMath::Clamp(InBlendFraction, 0.0f, 1.0f);

	if (BlendFraction > 0 && BlendDestinationProcessedTexture != NULL)
	{
		if (BlendFraction < 1)
		{
			IrradianceEnvironmentMap = (*InIrradianceEnvironmentMap) * (1 - BlendFraction) + (*BlendDestinationIrradianceEnvironmentMap) * BlendFraction;
			AverageBrightness = *InAverageBrightness * (1 - BlendFraction) + (*BlendDestinationAverageBrightness) * BlendFraction;
		}
		else
		{
			// Blend is full destination, treat as source to avoid blend overhead in shaders
			IrradianceEnvironmentMap = *BlendDestinationIrradianceEnvironmentMap;
			AverageBrightness = *BlendDestinationAverageBrightness;
		}
	}
	else
	{
		// Blend is full source
		IrradianceEnvironmentMap = *InIrradianceEnvironmentMap;
		AverageBrightness = *InAverageBrightness;
		BlendFraction = 0;
	}
}

FSkyLightSceneProxy::FSkyLightSceneProxy(const class USkyLightComponent* InLightComponent)
	: LightComponent(InLightComponent)
	, ProcessedTexture(InLightComponent->ProcessedSkyTexture->GetShaderResourceView())
	, BlendDestinationProcessedTexture(InLightComponent->BlendDestinationProcessedSkyTexture.Get())
	, SkyDistanceThreshold(InLightComponent->SkyDistanceThreshold)
	, bCastShadows(InLightComponent->CastShadows)
	, bWantsStaticShadowing(InLightComponent->Mobility == EComponentMobility::Stationary)
	, bHasStaticLighting(InLightComponent->HasStaticLighting())
	, bCastVolumetricShadow(InLightComponent->bCastVolumetricShadow)
	, LightColor(FLinearColor(InLightComponent->LightColor) * InLightComponent->Intensity)
	, IndirectLightingIntensity(InLightComponent->IndirectLightingIntensity)
	, VolumetricScatteringIntensity(FMath::Max(InLightComponent->VolumetricScatteringIntensity, 0.0f))
	, OcclusionMaxDistance(InLightComponent->OcclusionMaxDistance)
	, Contrast(InLightComponent->Contrast)
	, OcclusionExponent(FMath::Clamp(InLightComponent->OcclusionExponent, .1f, 10.0f))
	, MinOcclusion(FMath::Clamp(InLightComponent->MinOcclusion, 0.0f, 1.0f))
	, OcclusionTint(InLightComponent->OcclusionTint)
	, OcclusionCombineMode(InLightComponent->OcclusionCombineMode)
{
	Initialize(InLightComponent->BlendFraction, &InLightComponent->IrradianceEnvironmentMap, &InLightComponent->BlendDestinationIrradianceEnvironmentMap, &InLightComponent->AverageBrightness, &InLightComponent->BlendDestinationAverageBrightness);
}


void USkyLightComponent::UpdateSkyCaptureContents(UWorld* WorldToUpdate)
{
	if (SkyCapturesToUpdate.size() > 0)
	{
		UpdateSkyCaptureContentsArray(WorldToUpdate, SkyCapturesToUpdate, true);
	}

// 	if (SkyCapturesToUpdateBlendDestinations.Num() > 0)
// 	{
// 		UpdateSkyCaptureContentsArray(WorldToUpdate, SkyCapturesToUpdateBlendDestinations, false);
// 	}

}

void USkyLightComponent::UpdateSkyCaptureContentsArray(UWorld* WorldToUpdate, std::vector<USkyLightComponent*>& ComponentArray, bool bOperateOnBlendSource)
{
	for (int32 CaptureIndex = ComponentArray.size() - 1; CaptureIndex >= 0; CaptureIndex--)
	{
		USkyLightComponent* CaptureComponent = ComponentArray[CaptureIndex];
		AActor* Owner = CaptureComponent->GetOwner();
		if (CaptureComponent->SourceType != SLS_SpecifiedCubemap || CaptureComponent->Cubemap)
		{
			if (bOperateOnBlendSource)
			{
				CaptureComponent->ProcessedSkyTexture = RHICreateTextureCube(CaptureComponent->CubemapResolution, PF_FloatRGBA, FMath::CeilLogTwo(CaptureComponent->CubemapResolution) + 1, 0);
				WorldToUpdate->Scene->UpdateSkyCaptureContents(CaptureComponent, CaptureComponent->bCaptureEmissiveOnly, CaptureComponent->Cubemap.get(), CaptureComponent->ProcessedSkyTexture.get(), CaptureComponent->AverageBrightness, CaptureComponent->IrradianceEnvironmentMap, NULL);
			}
			else
			{

			}
		}
		CaptureComponent->MarkRenderStateDirty();
	}
}

std::vector<USkyLightComponent*> USkyLightComponent::SkyCapturesToUpdate;

std::vector<USkyLightComponent*> USkyLightComponent::SkyCapturesToUpdateBlendDestinations;

void USkyLightComponent::OnRegister()
{
	ULightComponentBase::OnRegister();
	SkyCapturesToUpdate.push_back(this);
}

void USkyLightComponent::CreateRenderState_Concurrent()
{
	ULightComponentBase::CreateRenderState_Concurrent();

	SceneProxy = CreateSceneProxy();

	if (SceneProxy)
	{
		// Add the light to the scene.
		GetWorld()->Scene->SetSkyLight(SceneProxy);
	}
}

void USkyLightComponent::DestroyRenderState_Concurrent()
{
	ULightComponentBase::DestroyRenderState_Concurrent();

	if (SceneProxy)
	{
		GetWorld()->Scene->DisableSkyLight(SceneProxy);
		delete SceneProxy;
		SceneProxy = NULL;
	}
}

ASkylight::ASkylight(class UWorld* InOwner)
	: AActor(InOwner)
{
	SkylightComponent = new USkyLightComponent(this);
}

void ASkylight::PostLoad()
{
	SkylightComponent->Register();
}
