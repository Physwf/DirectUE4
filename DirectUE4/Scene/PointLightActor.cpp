#pragma once

#include "PointLightActor.h"

PointLightActor::PointLightActor(class UWorld* InWorld)
	: AActor(InWorld)
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
