#pragma once

#include "UnrealMath.h"
#include "Actor.h"

class PointLight : public Actor
{
	//Vector4 PoistionAndRadias;
	//Vector4 ColorAndFalloffExponent;
	virtual void PostLoad() override {}
	virtual void Tick(float fDeltaSeconds) override {};
};

class DirectionalLight : public Actor
{
public:
	FVector Direction;
	float Intencity;
	FVector Color;
	float LightSourceAngle;		// Defaults to 0.5357 which is the angle for our sun.
	float LightSourceSoftAngle;	//0.0
	virtual void PostLoad() override {}
	virtual void Tick(float fDeltaSeconds) override {};
};

