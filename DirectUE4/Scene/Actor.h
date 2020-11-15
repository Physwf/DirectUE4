#pragma once

#include "UnrealMath.h"

class Actor
{
public:
	Actor();
	virtual ~Actor();

	virtual void Tick(float fDeltaSeconds) = 0;

	void SetPosition(Vector InPosition);
	void SetRotation(Rotator InRotation);
protected:
	Matrix GetWorldMatrix();

	Vector Position;
	Rotator Rotation;
	bool bTransformDirty = false;
};