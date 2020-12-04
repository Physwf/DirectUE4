#include "PrimitiveSceneProxy.h"
#include "PrimitiveComponent.h"
#include "PrimitiveSceneInfo.h"

FPrimitiveSceneProxy::FPrimitiveSceneProxy(const UPrimitiveComponent* InComponent)
	:
	Mobility(InComponent->Mobility)
	, bStaticLighting(false)
	//, bRenderInMainPass(true)
	, bHasValidSettingsForStaticLighting(true)
	, bCastDynamicShadow(true)
	, bCastStaticShadow(true)
	, bCastVolumetricTranslucentShadow(false)
	, bSelfShadowOnly(false)
	, bCastInsetShadow(false)	// Assumed to be enabled if bSelfShadowOnly is enabled.
	, bCastFarShadow(true)
	//, Scene(InComponent->GetScene())
	//, PrimitiveComponentId(0)
	, PrimitiveSceneInfo(NULL)
{
	UniformBuffer.InitDynamicRHI();
}

FPrimitiveSceneProxy::~FPrimitiveSceneProxy()
{
	UniformBuffer.ReleaseDynamicRHI();
}

FPrimitiveViewRelevance FPrimitiveSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	return FPrimitiveViewRelevance();
}

void FPrimitiveSceneProxy::UpdateUniformBuffer()
{
	const FPrimitiveUniformShaderParameters PrimitiveUniformShaderParameters =
		GetPrimitiveUniformShaderParameters(
			LocalToWorld,
			ActorPosition,
			Bounds,
			LocalBounds,
			false,//bReceivesDecals,
			false,//HasDistanceFieldRepresentation(),
			false,//HasDynamicIndirectShadowCasterRepresentation(),
			false,//UseSingleSampleShadowFromStationaryLights(),
			false,//Scene->HasPrecomputedVolumetricLightmap_RenderThread(),
			false,//UseEditorDepthTest(),
			1,//GetLightingChannelMask(),
			1.f//LpvBiasMultiplier
			);
	UniformBuffer.SetContents(PrimitiveUniformShaderParameters);
}

void FPrimitiveSceneProxy::SetTransform(const FMatrix& InLocalToWorld, const FBoxSphereBounds& InBounds, const FBoxSphereBounds& InLocalBounds, FVector InActorPosition)
{
	LocalToWorld = InLocalToWorld;
	bIsLocalToWorldDeterminantNegative = LocalToWorld.Determinant() < 0.0f;

	// Update the cached bounds.
	Bounds = InBounds;
	LocalBounds = InLocalBounds;
	ActorPosition = InActorPosition;

	UpdateUniformBufferMaybeLazy();
}

void FPrimitiveSceneProxy::UpdateUniformBufferMaybeLazy()
{
	PrimitiveSceneInfo->SetNeedsUniformBufferUpdate(true);
}

