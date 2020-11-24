#pragma once

#include "Actor.h"
#include "LightComponent.h"

class DirectionalLightActor : public Actor
{
public:
	DirectionalLightActor(class UWorld* InWorld);
	~DirectionalLightActor();
	
};

