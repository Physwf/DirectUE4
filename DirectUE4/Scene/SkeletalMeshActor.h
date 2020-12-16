#pragma once

#include "Actor.h"

class USkeletalMeshComponent;
class UAnimSequence;

class SkeletalMeshActor : public AActor
{
public:
	SkeletalMeshActor(class UWorld* InOwner, const char* ResourcePath);
	SkeletalMeshActor(class UWorld* InOwner, const char* ResourcePath, const char* AnimationPath);
	virtual ~SkeletalMeshActor();

	virtual void PostLoad() override;

	virtual void Tick(float fDeltaTime) override;
protected:
	USkeletalMeshComponent* MeshComponent;
	UAnimSequence* AnimSequence;
};