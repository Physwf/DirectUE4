#pragma once

#include "Actor.h"

class USkeletalMesh;

class SkeletalMeshActor : public AActor
{
public:
	SkeletalMeshActor(class UWorld* InOwner, const char* ResourcePath);
	virtual ~SkeletalMeshActor();

	virtual void PostLoad() override;

	virtual void Tick(float fDeltaTime) override;
private:
	USkeletalMesh * Mesh;
};