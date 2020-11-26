#include "SceneComponent.h"
#include "World.h"
#include "Actor.h"

USceneComponent::USceneComponent(AActor* InOwner) :Owner(InOwner)
{
	WorldPrivite = Owner->GetWorld();
}

USceneComponent::~USceneComponent()
{

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
	return Owner;
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
