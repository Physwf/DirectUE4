#pragma once

#include "UnrealMath.h"
#include "Actor.h"

class PointLight : public Actor
{
	//Vector4 PoistionAndRadias;
	//Vector4 ColorAndFalloffExponent;

	virtual void Tick(float fDeltaSeconds) override {};
};

class DirectionalLight : public Actor
{
public:
	Vector Direction;
	float Intencity;
	Vector Color;
	float LightSourceAngle;		// Defaults to 0.5357 which is the angle for our sun.
	float LightSourceSoftAngle;	//0.0

	virtual void Tick(float fDeltaSeconds) override {};
	virtual void Register() override {};
	virtual void UnRegister() override {};
};

