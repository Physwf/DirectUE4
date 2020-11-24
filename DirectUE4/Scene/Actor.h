#pragma once

#include "UnrealMath.h"

class UWorld;

class Actor
{
public:
	Actor(UWorld* InOwner);
	virtual ~Actor();

	virtual void Tick(float fDeltaSeconds) = 0;
	virtual void PostLoad() = 0;

	void SetPosition(FVector InPosition);
	FVector GetPosition() { return Position; }
	void SetRotation(FRotator InRotation);
	FRotator GetRotation() { return Rotation; }

	class UWorld* GetWorld() { return WorldPrivite; }

	void DoDeferredRenderUpdates_Concurrent();

	virtual void SendRenderTransform_Concurrent();
	
	class UPrimitiveComponent* Proxy;

	FMatrix GetWorldMatrix();

protected:

	FVector Position;
	FRotator Rotation;
	bool bTransformDirty = false;

	UWorld* WorldPrivite;

	friend class UWorld;
};