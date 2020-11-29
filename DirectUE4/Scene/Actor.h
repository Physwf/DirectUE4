#pragma once

#include "UnrealMath.h"
#include "SceneComponent.h"

class UWorld;

class AActor
{
public:
	AActor(UWorld* InOwner);
	virtual ~AActor();

	virtual void Tick(float fDeltaSeconds) = 0;
	virtual void PostLoad() = 0;

	void SetActorLocation(FVector NewLocation);
	FVector GetActorLocation() const { return Position; }
	void SetActorRotation(FRotator InRotation);
	FRotator GetActorRotation() { return Rotation; }

	FQuat GetActorQuat() const
	{
		return RootComponent ? RootComponent->GetComponentQuat() : FQuat(0,0,0,1.f);
	}

	class UWorld* GetWorld() { return WorldPrivite; }
protected:
	class USceneComponent* RootComponent;

	FVector Position;
	FRotator Rotation;
	bool bTransformDirty = false;

	UWorld* WorldPrivite;

	friend class UWorld;
};