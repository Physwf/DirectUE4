#pragma once

#include "UnrealMath.h"

class Actor
{
public:
	Actor();
	virtual ~Actor();

	virtual void Tick(float fDeltaSeconds) = 0;
	virtual void PostLoad() = 0;

	void SetPosition(FVector InPosition);
	FVector GetPosition() { return Position; }
	void SetRotation(FRotator InRotation);
	FRotator GetRotation() { return Rotation; }

	class World* GetWorld() { return WorldPrivite; }

	void DoDeferredRenderUpdates_Concurrent();

	virtual void SendRenderTransform_Concurrent();
	
	class MeshPrimitive* Proxy;

	FMatrix GetWorldMatrix();

protected:

	FVector Position;
	FRotator Rotation;
	bool bTransformDirty = false;

	class World* WorldPrivite;

	friend class World;
};