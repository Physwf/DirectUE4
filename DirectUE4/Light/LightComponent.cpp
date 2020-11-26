#include "LightComponent.h"
#include "World.h"
#include "Scene.h"
#include "SceneManagement.h"

ULightComponent::ULightComponent(class AActor* InOwner)
	: USceneComponent(InOwner)
{

}

ULightComponent::~ULightComponent()
{

}

bool ULightComponent::AffectsPrimitive(const UPrimitiveComponent* Primitive) const
{
	//return AffectsBounds(Primitive->Bounds);
	return true;
}

bool ULightComponent::AffectsBounds(const FBoxSphereBounds& InBounds) const
{
	return true;
}

FVector ULightComponent::GetDirection() const
{
	return GetComponentTransform().GetUnitAxis(EAxis::X);
}

bool ULightComponent::HasStaticLighting() const
{
	return false;
}

bool ULightComponent::HasStaticShadowing() const
{
	return !bMoveable;
}

void ULightComponent::Register()
{
	GetWorld()->Scene->AddLight(this);
}

void ULightComponent::Unregister()
{
	GetWorld()->Scene->RemoveLight(this);
}
static float GMaxCSMRadiusToAllowPerObjectShadows = 8000;
class FDirectionalLightSceneProxy : public FLightSceneProxy
{
public:
	/**
	* Radius of the whole scene dynamic shadow centered on the viewer, which replaces the precomputed shadows based on distance from the camera.
	* A Radius of 0 disables the dynamic shadow.
	*/
	float WholeSceneDynamicShadowRadius;
	/**
	* Number of cascades to split the view frustum into for the whole scene dynamic shadow.
	* More cascades result in better shadow resolution and allow WholeSceneDynamicShadowRadius to be further, but add rendering cost.
	*/
	uint32 DynamicShadowCascades;

	bool bUseInsetShadowsForMovableObjects;

	FDirectionalLightSceneProxy(const UDirectionalLightComponent* Component) :
		FLightSceneProxy(Component)
	{

	}

};

UDirectionalLightComponent::UDirectionalLightComponent(AActor* InOwner)
	: ULightComponent(InOwner)
{

}

UDirectionalLightComponent::~UDirectionalLightComponent()
{

}

Vector4 UDirectionalLightComponent::GetLightPosition() const
{
	return Vector4(-GetDirection() * WORLD_MAX, 0);
}

ELightComponentType UDirectionalLightComponent::GetLightType() const
{
	return LightType_Directional;
}

FLightSceneProxy* UDirectionalLightComponent::CreateSceneProxy() const
{
	return new FDirectionalLightSceneProxy(this);
}

ULocalLightComponent::ULocalLightComponent(AActor* InOwner)
	: ULightComponent(InOwner)
{

}

ULocalLightComponent::~ULocalLightComponent()
{

}

bool ULocalLightComponent::AffectsBounds(const FBoxSphereBounds& InBounds) const
{
	if ((InBounds.Origin - GetComponentTransform().GetLocation()).SizeSquared() > FMath::Square(AttenuationRadius + InBounds.SphereRadius))
	{
		return false;
	}

	if (!ULightComponent::AffectsBounds(InBounds))
	{
		return false;
	}

	return true;
}

Vector4 ULocalLightComponent::GetLightPosition() const
{
	return Vector4(GetComponentTransform().GetLocation(), 1);
}

FBox ULocalLightComponent::GetBoundingBox() const
{
	return FBox(GetComponentLocation() - FVector(AttenuationRadius, AttenuationRadius, AttenuationRadius), GetComponentLocation() + FVector(AttenuationRadius, AttenuationRadius, AttenuationRadius));
}

FSphere ULocalLightComponent::GetBoundingSphere() const
{
	return FSphere(GetComponentTransform().GetLocation(), AttenuationRadius);
}

UPointLightComponent::UPointLightComponent(AActor* InOwner)
	: ULocalLightComponent(InOwner)
{

}

UPointLightComponent::~UPointLightComponent()
{

}

ELightComponentType UPointLightComponent::GetLightType() const
{
	return LightType_Point;
}

FLightSceneProxy* UPointLightComponent::CreateSceneProxy() const
{
	return new FPointLightSceneProxy(this);
}

bool FPointLightSceneProxy::GetWholeSceneProjectedShadowInitializer(const FSceneViewFamily& ViewFamily, std::vector<FWholeSceneProjectedShadowInitializer>& OutInitializers) const
{
	OutInitializers.push_back(FWholeSceneProjectedShadowInitializer());
	FWholeSceneProjectedShadowInitializer& OutInitializer = OutInitializers.back();
	OutInitializer.PreShadowTranslation = -GetLightToWorld().GetOrigin();
	OutInitializer.WorldToLight = GetWorldToLight().RemoveTranslation();
	OutInitializer.Scales = FVector(1, 1, 1);
	OutInitializer.FaceDirection = FVector(0, 0, 1);
	OutInitializer.SubjectBounds = FBoxSphereBounds(FVector(0, 0, 0), FVector(Radius, Radius, Radius), Radius);
	OutInitializer.WAxis = Vector4(0, 0, 1, 0);
	OutInitializer.MinLightW = 0.1f;
	OutInitializer.MaxDistanceToCastInLightW = Radius;
	OutInitializer.bOnePassPointLightShadow = true;
	//OutInitializer.bRayTracedDistanceField = UseRayTracedDistanceFieldShadows() && DoesPlatformSupportDistanceFieldShadowing(ViewFamily.GetShaderPlatform());
	return true;
}
