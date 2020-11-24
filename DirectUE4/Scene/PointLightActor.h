#pragma once

#include "Actor.h"
#include "LightComponent.h"

class PointLightActor : public Actor 
{
public:
	PointLightActor(class UWorld* InWorld);
	~PointLightActor();

	void PostLoad();
private:
	UPointLightComponent* Componet;
};


