#pragma once

#include "PointLightActor.h"

PointLightActor::PointLightActor(class UWorld* InWorld)
	: Actor(InWorld)
{
	Componet = new UPointLightComponent(this);
}

PointLightActor::~PointLightActor()
{
	delete Componet;
}

void PointLightActor::PostLoad()
{
	Componet->Register();
}
