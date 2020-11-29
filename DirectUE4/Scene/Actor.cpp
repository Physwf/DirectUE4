#include "Actor.h"
#include "SceneComponent.h"

AActor::AActor(UWorld* InOwner)
	: WorldPrivite(InOwner), Position{ 0, 0, 0 }, Rotation{ 0, 0, 0 }
{
	RootComponent = nullptr;
}

AActor::~AActor()
{

}

void AActor::SetActorLocation(FVector NewLocation)
{
	if (RootComponent)
	{
		const FVector Delta = NewLocation - GetActorLocation();
		RootComponent->MoveComponent(Delta, GetActorQuat());
	}
	Position = NewLocation;

}

void AActor::SetActorRotation(FRotator InRotation)
{
	Rotation = InRotation;
}
