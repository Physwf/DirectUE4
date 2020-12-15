#pragma once

#include "UnrealMath.h"
#include "SceneComponent.h"

class UWorld;

class AActor
{
public:
	AActor(UWorld* InOwner);
	virtual ~AActor();

	virtual void Tick(float fDeltaSeconds) = 0;
	virtual void PostLoad() = 0;

	FTransform ActorToWorld() const
	{
		return (RootComponent ? RootComponent->GetComponentTransform() : FTransform::Identity);
	}

	FTransform GetTransform() const
	{
		return ActorToWorld();
	}

	template<class T>
	static inline FVector TemplateGetActorLocation(const T* RootComponent)
	{
		return (RootComponent != nullptr) ? RootComponent->GetComponentLocation() : FVector::ZeroVector;
	}
	template<class T>
	static inline FTransform TemplateGetActorTransform(const T* RootComponent)
	{
		return (RootComponent != nullptr) ? RootComponent->GetComponentTransform() : FTransform();
	}
	template<class T>
	static inline FRotator TemplateGetActorRotation(const T* RootComponent)
	{
		return (RootComponent != nullptr) ? RootComponent->GetComponentRotation() : FRotator::ZeroRotator;
	}

	bool SetActorLocation(FVector NewLocation);
	FVector GetActorLocation() const 
	{ 
		return TemplateGetActorLocation(RootComponent);
	}
	bool SetActorRotation(FRotator InRotation);
	FRotator GetActorRotation() 
	{ 
		return TemplateGetActorRotation(RootComponent);
	}

	FTransform GetActorTransform() const
	{
		return TemplateGetActorTransform(RootComponent);
	}

	FQuat GetActorQuat() const
	{
		return RootComponent ? RootComponent->GetComponentQuat() : FQuat(0,0,0,1.f);
	}

	class UWorld* GetWorld() { return WorldPrivite; }
protected:
	class USceneComponent* RootComponent;

	FVector Position;
	FRotator Rotation;
	bool bTransformDirty = false;

	UWorld* WorldPrivite;

	friend class UWorld;
};