#pragma once

#include "UnrealMath.h"
#include "Transform.h"

class UWorld;
class Actor;

class USceneComponent
{
public:
	USceneComponent(Actor* InOwner);
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

	virtual void Register() = 0;
	virtual void Unregister() = 0;

	UWorld* GetWorld() { return WorldPrivite; }
private:
	UWorld* WorldPrivite;
	Actor* Owner;
};

