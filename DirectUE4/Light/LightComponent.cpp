#include "LightComponent.h"
#include "World.h"
#include "Scene.h"
#include "SceneManagement.h"

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

class FDirectionalLightSceneProxy : public FLightSceneProxy
{
public:

	FDirectionalLightSceneProxy(const UDirectionalLightComponent* Component) :
		FLightSceneProxy(Component)
	{

	}
};

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

FLightSceneProxy* UDirectionalLightComponent::CreateSceneProxy() const
{
	return new FDirectionalLightSceneProxy(this);
}
/** The parts of the point light scene info that aren't dependent on the light policy type. */
class FLocalLightSceneProxy : public FLightSceneProxy
{
public:
	/** The light radius. */
	float Radius;
	/** One over the light's radius. */
	float InvRadius;

	/** Initialization constructor. */
	FLocalLightSceneProxy(const ULocalLightComponent* Component)
		: FLightSceneProxy(Component)
		, MaxDrawDistance(Component->MaxDrawDistance)
		, FadeRange(Component->MaxDistanceFadeRange)
	{
		UpdateRadius(Component->AttenuationRadius);
	}
	// FLightSceneInfo interface.
	virtual float GetMaxDrawDistance() const final override
	{
		return MaxDrawDistance;
	}
	virtual float GetFadeRange() const final override
	{
		return FadeRange;
	}
	/** @return radius of the light or 0 if no radius */
	virtual float GetRadius() const override
	{
		return Radius;
	}
	virtual bool AffectsBounds(const FBoxSphereBounds& Bounds) const override
	{
		if ((Bounds.Origin - GetLightToWorld().GetOrigin()).SizeSquared() > FMath::Square(Radius + Bounds.SphereRadius))
		{
			return false;
		}

		if (!FLightSceneProxy::AffectsBounds(Bounds))
		{
			return false;
		}

		return true;
	}
	virtual FSphere GetBoundingSphere() const
	{
		return FSphere(GetPosition(), GetRadius());
	}
	virtual FVector GetPerObjectProjectedShadowProjectionPoint(const FBoxSphereBounds& SubjectBounds) const
	{
		return GetOrigin();
	}
protected:
	/** Updates the light scene info's radius from the component. */
	void UpdateRadius(float ComponentRadius)
	{
		Radius = ComponentRadius;

		// Min to avoid div by 0 (NaN in InvRadius)
		InvRadius = 1.0f / FMath::Max(0.00001f, ComponentRadius);
	}
	float MaxDrawDistance;
	float FadeRange;
};

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

class FPointLightSceneProxy : public FLocalLightSceneProxy
{
public:
	/** The light falloff exponent. */
	float FalloffExponent;

	/** Radius of light source shape */
	float SourceRadius;

	/** Soft radius of light source shape */
	float SoftSourceRadius;

	/** Length of light source shape */
	float SourceLength;

	/** Whether light uses inverse squared falloff. */
	const uint32 bInverseSquared : 1;

	/** Initialization constructor. */
	FPointLightSceneProxy(const UPointLightComponent* Component)
		: FLocalLightSceneProxy(Component)
		, FalloffExponent(Component->LightFalloffExponent)
		, SourceRadius(Component->SourceRadius)
		, SoftSourceRadius(Component->SoftSourceRadius)
		, SourceLength(Component->SourceLength)
		, bInverseSquared(Component->bUseInverseSquaredFalloff)
	{
		UpdateRadius(Component->AttenuationRadius);
	}

	virtual float GetSourceRadius() const override
	{
		return SourceRadius;
	}

	virtual bool IsInverseSquared() const override
	{
		return bInverseSquared;
	}
};

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

FLightSceneProxy* UPointLightComponent::CreateSceneProxy() const
{
	return new FPointLightSceneProxy(this);
}

