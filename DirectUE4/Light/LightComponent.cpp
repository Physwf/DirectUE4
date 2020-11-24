#include "LightComponent.h"
#include "World.h"
#include "Scene.h"

ULightComponent::ULightComponent(class Actor* InOwner)
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

void ULightComponent::Register()
{
	GetWorld()->Scene->AddLight(this);
}

void ULightComponent::Unregister()
{
	GetWorld()->Scene->RemoveLight(this);
}

UDirectionalLightComponent::UDirectionalLightComponent(Actor* InOwner)
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

ULocalLightComponent::ULocalLightComponent(Actor* InOwner)
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

UPointLightComponent::UPointLightComponent(Actor* InOwner)
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

