#include "SceneComponent.h"
#include "World.h"
#include "Actor.h"

USceneComponent::USceneComponent(Actor* InOwner) :Owner(InOwner)
{
	WorldPrivite = Owner->GetWorld();
}

USceneComponent::~USceneComponent()
{

}

