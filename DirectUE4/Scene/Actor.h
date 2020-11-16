#pragma once

#include "UnrealMath.h"

class Actor
{
public:
	Actor();
	virtual ~Actor();

	virtual void Tick(float fDeltaSeconds) = 0;
	virtual void PostLoad() = 0;

	void SetPosition(Vector InPosition);
	void SetRotation(FRotator InRotation);

	class World* GetWorld() { return WorldPrivite; }
protected:
	Matrix GetWorldMatrix();

	Vector Position;
	FRotator Rotation;
	bool bTransformDirty = false;

	class World* WorldPrivite;

	friend class World;
};