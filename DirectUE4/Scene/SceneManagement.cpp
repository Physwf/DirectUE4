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
