#include "SkyLight.h"
#include "SceneManagement.h"
#include "World.h"
#include "Scene.h"

USkyLightComponent::USkyLightComponent(AActor* InOwner)
	: ULightComponentBase(InOwner)
{

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

}

FSkyLightSceneProxy::FSkyLightSceneProxy(const class USkyLightComponent* InLightComponent)
	: LightComponent(InLightComponent)
	, ProcessedTexture(InLightComponent->ProcessedSkyTexture.Get())
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

}


void USkyLightComponent::UpdateSkyCaptureContents(UWorld* WorldToUpdate)
{

}

void USkyLightComponent::UpdateSkyCaptureContentsArray(UWorld* WorldToUpdate, std::vector<USkyLightComponent*>& ComponentArray, bool bBlendSources)
{

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
