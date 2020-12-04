#pragma once

#include "PointLightActor.h"

PointLightActor::PointLightActor(class UWorld* InWorld)
	: AActor(InWorld)
{
	Componet = new UPointLightComponent(this);
	Componet->Mobility = EComponentMobility::Movable;
	RootComponent = Componet;
}

PointLightActor::~PointLightActor()
{
	delete Componet;
}

void PointLightActor::PostLoad()
{
	Componet->Register();
}

void PointLightActor::Tick(float fDeltaSeconds)
{

}
