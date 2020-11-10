#pragma once

#include "UnrealMath.h"

class World;

class Actor
{
public:
	Actor();
	virtual ~Actor();

	virtual void Tick(float fDeltaSeconds) = 0;
	virtual void Register() = 0;
	virtual void UnRegister() = 0;

	void SetPosition(Vector InPosition);
	void SetRotation(Rotator InRotation);

	World* GetWorld() { return mWorld; }
protected:
	Matrix GetWorldMatrix();

	Vector Position;
	Rotator Rotation;
	bool bTransformDirty = false;

	World* mWorld;

	friend class World;
};