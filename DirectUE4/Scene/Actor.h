#pragma once

#include "UnrealMath.h"

class UWorld;

class AActor
{
public:
	AActor(UWorld* InOwner);
	virtual ~AActor();

	virtual void Tick(float fDeltaSeconds) = 0;
	virtual void PostLoad() = 0;

	void SetActorLocation(FVector InPosition);
	FVector GetActorLocation() const { return Position; }
	void SetActorRotation(FRotator InRotation);
	FRotator GetActorRotation() { return Rotation; }

	class UWorld* GetWorld() { return WorldPrivite; }

	FMatrix GetWorldMatrix();
protected:

	FVector Position;
	FRotator Rotation;
	bool bTransformDirty = false;

	UWorld* WorldPrivite;

	friend class UWorld;
};