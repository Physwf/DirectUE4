#pragma once

#include "Actor.h"
#include "LightComponent.h"

class PointLightActor : public AActor 
{
public:
	PointLightActor(class UWorld* InWorld);
	~PointLightActor();

	void PostLoad();

	virtual void Tick(float fDeltaSeconds) override;
private:
	UPointLightComponent* Componet;
};


