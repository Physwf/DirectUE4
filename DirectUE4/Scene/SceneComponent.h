#pragma once

#include "UnrealMath.h"
#include "Transform.h"

class UWorld;
class AActor;


class UActorComponent
{
protected:
	/** Is this component's transform in need of sending to the renderer? */
	uint8 bRenderTransformDirty : 1;

	/** Is this component's dynamic data in need of sending to the renderer? */
	uint8 bRenderDynamicDataDirty : 1;
public:
	void DoDeferredRenderUpdates_Concurrent();

	virtual void SendRenderTransform_Concurrent() {};

	virtual void SendRenderDynamicData_Concurrent() {};
};

class USceneComponent : public UActorComponent
{
public:
	USceneComponent(AActor* InOwner);
	virtual ~USceneComponent();

	FBoxSphereBounds Bounds;
	/** Location of the component relative to its parent */
	FVector RelativeLocation;
	/** Rotation of the component relative to its parent */
	FRotator RelativeRotation;
	FVector RelativeScale3D;

	FVector ComponentVelocity;

private:
	/** Current transform of the component, relative to the world */
	FTransform ComponentToWorld;
public:
	inline const FTransform& GetComponentTransform() const
	{
		return ComponentToWorld;
	}
	FVector GetComponentLocation() const
	{
		return GetComponentTransform().GetLocation();
	}

	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const;

	AActor* GetAttachmentRootActor() const;

	virtual void Register() = 0;
	virtual void Unregister() = 0;

	UWorld* GetWorld() { return WorldPrivite; }
private:
	UWorld* WorldPrivite;
	AActor* Owner;
};

