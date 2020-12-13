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

bool AActor::SetActorLocation(FVector NewLocation)
{
	if (RootComponent)
	{
		const FVector Delta = NewLocation - GetActorLocation();
		return RootComponent->MoveComponent(Delta, GetActorQuat());
	}
	Position = NewLocation;
	return false;
}

bool AActor::SetActorRotation(FRotator NewRotation)
{
	if (RootComponent)
	{
		return RootComponent->MoveComponent(FVector::ZeroVector, NewRotation.Quaternion());
	}
	Rotation = NewRotation;
	return false;
}
