#include "SceneComponent.h"
#include "World.h"
#include "Actor.h"

UActorComponent::UActorComponent(AActor* InOwner)
	: Owner(InOwner)
{
	WorldPrivite = Owner->GetWorld();
}

UActorComponent::~UActorComponent()
{

}

void UActorComponent::OnRegister()
{
	GetWorld()->RegisterComponent(this);
}

void UActorComponent::OnUnregister()
{
	GetWorld()->UnregisterComponent(this);
}

void UActorComponent::Register()
{
	CreateRenderState_Concurrent();
}

void UActorComponent::Unregister()
{
	DestroyRenderState_Concurrent();
}

void UActorComponent::CreateRenderState_Concurrent()
{

}

void UActorComponent::DestroyRenderState_Concurrent()
{

}

void UActorComponent::DoDeferredRenderUpdates_Concurrent()
{
	if (bRenderTransformDirty)
	{
		// Update the component's transform if the actor has been moved since it was last updated.
		SendRenderTransform_Concurrent();
	}

	if (bRenderDynamicDataDirty)
	{
		SendRenderDynamicData_Concurrent();
	}
}


USceneComponent::USceneComponent(AActor* InOwner) : UActorComponent(InOwner)
{
	
}

USceneComponent::~USceneComponent()
{

}

bool USceneComponent::MoveComponent(const FVector& Delta, const FQuat& NewRotation)
{
	ComponentToWorld = FTransform(NewRotation, Delta, FVector::OneVector);
	return true;
}

void USceneComponent::ConditionalUpdateComponentToWorld()
{

}

bool USceneComponent::InternalSetWorldLocationAndRotation(FVector NewLocation, const FQuat& RotationQuat)
{
	return true;
}

FBoxSphereBounds USceneComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds NewBounds;
	NewBounds.Origin = LocalToWorld.GetLocation();
	NewBounds.BoxExtent = FVector::ZeroVector;
	NewBounds.SphereRadius = 0.f;
	return NewBounds;
}

AActor* USceneComponent::GetAttachmentRootActor() const
{
	return GetOwner();
}

