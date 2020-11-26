#pragma once

#include "Actor.h"
#include "LightComponent.h"

class DirectionalLightActor : public AActor
{
public:
	DirectionalLightActor(class UWorld* InWorld);
	~DirectionalLightActor();
	
};

