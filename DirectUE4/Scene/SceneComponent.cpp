#include "SceneComponent.h"
#include "World.h"
#include "Actor.h"

UActorComponent::UActorComponent(AActor* InOwner)
	: Owner(InOwner)
	, bRegistered(false)
	, bRenderStateCreated(false)
	, bRenderStateDirty(false)
	, bRenderTransformDirty(false)
	, bRenderDynamicDataDirty(false)
{
	WorldPrivate = Owner->GetWorld();
}

UActorComponent::~UActorComponent()
{

}

void UActorComponent::OnRegister()
{
	bRegistered = true;
	GetWorld()->RegisterComponent(this);
}

void UActorComponent::OnUnregister()
{
	GetWorld()->UnregisterComponent(this);
	bRegistered = false;
}

void UActorComponent::Register()
{
	OnRegister();
	CreateRenderState_Concurrent();
}

void UActorComponent::Unregister()
{
	DestroyRenderState_Concurrent();
	OnUnregister();
}

void UActorComponent::CreateRenderState_Concurrent()
{
	assert(IsRegistered());
	//assert(WorldPrivate->Scene);
	assert(!bRenderStateCreated);
	bRenderStateCreated = true;

	bRenderStateDirty = false;
	bRenderTransformDirty = false;
	bRenderDynamicDataDirty = false;
}

void UActorComponent::SendRenderTransform_Concurrent()
{
	assert(bRenderStateCreated);
	bRenderTransformDirty = false;
}

void UActorComponent::SendRenderDynamicData_Concurrent()
{
	assert(bRenderStateCreated);
	bRenderDynamicDataDirty = false;
}

void UActorComponent::DestroyRenderState_Concurrent()
{
	assert(bRenderStateCreated);
	bRenderStateCreated = false;
}

void UActorComponent::RecreateRenderState_Concurrent()
{
	if (bRenderStateCreated)
	{
		assert(IsRegistered()); // Should never have render state unless registered
		DestroyRenderState_Concurrent();
		assert(!bRenderStateCreated/*, TEXT("Failed to route DestroyRenderState_Concurrent (%s)"), *GetFullName()*/);
	}

	if (IsRegistered() /*&& WorldPrivate->Scene*/)
	{
		CreateRenderState_Concurrent();
		assert(bRenderStateCreated/*, TEXT("Failed to route CreateRenderState_Concurrent (%s)"), *GetFullName()*/);
	}
}

void UActorComponent::DoDeferredRenderUpdates_Concurrent()
{
	if (bRenderStateDirty)
	{
		RecreateRenderState_Concurrent();
	}
	else
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

}


void UActorComponent::MarkRenderStateDirty()
{
	if (IsRegistered() && bRenderStateCreated && (!bRenderStateDirty || !GetWorld()))
	{
		// Flag as dirty
		bRenderStateDirty = true;
		MarkForNeededEndOfFrameRecreate();
	}
}

void UActorComponent::MarkRenderDynamicDataDirty()
{
	if (IsRegistered() && bRenderStateCreated)
	{
		// Flag as dirty
		bRenderDynamicDataDirty = true;
		MarkForNeededEndOfFrameUpdate();
	}
}

void UActorComponent::MarkRenderTransformDirty()
{
	if (IsRegistered() && bRenderStateCreated)
	{
		bRenderTransformDirty = true;
		MarkForNeededEndOfFrameUpdate();
	}
}

void UActorComponent::MarkForNeededEndOfFrameUpdate()
{
// 	if (bNeverNeedsRenderUpdate)
// 	{
// 		return;
// 	}
// 
// 	UWorld* ComponentWorld = GetWorld();
// 	if (ComponentWorld)
// 	{
// 		ComponentWorld->MarkActorComponentForNeededEndOfFrameUpdate(this, RequiresGameThreadEndOfFrameUpdates());
// 	}
// 	else if (!IsUnreachable())
	{
		// we don't have a world, do it right now.
		DoDeferredRenderUpdates_Concurrent();
	}
}

void UActorComponent::MarkForNeededEndOfFrameRecreate()
{
// 	if (bNeverNeedsRenderUpdate)
// 	{
// 		return;
// 	}
// 
// 	UWorld* ComponentWorld = GetWorld();
// 	if (ComponentWorld)
// 	{
// 		// by convention, recreates are always done on the gamethread
// 		ComponentWorld->MarkActorComponentForNeededEndOfFrameUpdate(this, RequiresGameThreadEndOfFrameRecreate());
// 	}
// 	else if (!IsUnreachable())
	{
		// we don't have a world, do it right now.
		DoDeferredRenderUpdates_Concurrent();
	}
}

class FScene* UActorComponent::GetScene() const
{
	return (WorldPrivate ? WorldPrivate->Scene : NULL);
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
	MarkRenderTransformDirty();
	return true;
}

void USceneComponent::ConditionalUpdateComponentToWorld()
{

}

bool USceneComponent::InternalSetWorldLocationAndRotation(FVector NewLocation, const FQuat& RotationQuat)
{
	return true;
}

FTransform USceneComponent::GetRelativeTransform() const
{
	return FTransform::Identity;
}

FBoxSphereBounds USceneComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds NewBounds;
	NewBounds.Origin = LocalToWorld.GetLocation();
	NewBounds.BoxExtent = FVector::ZeroVector;
	NewBounds.SphereRadius = 0.f;
	return NewBounds;
}

void USceneComponent::UpdateBounds()
{
	Bounds = CalcBounds(GetComponentTransform());
}

AActor* USceneComponent::GetAttachmentRootActor() const
{
	return GetOwner();
}

